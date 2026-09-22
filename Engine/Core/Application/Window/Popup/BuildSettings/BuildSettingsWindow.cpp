#include "BuildSettingsWindow.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <system_error>
#include <vector>

#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")

#include "ImGuiHelper.h"
#include "../../../Build/GameBuilder.h"
#include "../../../Configuration/Build/ApplicationConfiguration_Build.h"
#include "../../../../../Module/Asset/Scene/SceneFile.h"
#include "../../../../../Module/Log/NanamiEngine_Module_Log.h"

namespace
{
    const ImVec4    BUILD_SETTINGS_ERROR_COLOR   = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
    const ImVec4    BUILD_SETTINGS_SUCCESS_COLOR = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
    constexpr float BUILD_SETTINGS_LABEL_WIDTH   = 130.0f;

    std::string BuildSettingsPathToUtf8(const std::filesystem::path& path)
    {
        const std::u8string utf8 = path.u8string();
        return std::string(utf8.begin(), utf8.end());
    }

    void BuildSettingsLabel(const char* label)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(BUILD_SETTINGS_LABEL_WIDTH);
    }

    void BuildSettingsErrorText(const std::string& text)
    {
        ImGui::SetCursorPosX(BUILD_SETTINGS_LABEL_WIDTH);
        ImGui::PushStyleColor(ImGuiCol_Text, BUILD_SETTINGS_ERROR_COLOR);
        ImGui::TextWrapped("%s", text.c_str());
        ImGui::PopStyleColor();
    }
}

namespace NanamiEngine::Core::PopupWindow
{
    namespace
    {
        /** @brief 入力を確定したフレームで true を返す。確定した文字列は buffer に入っている */
        template <size_t N>
        bool BuildSettingsEditText(const char* id, char (&buffer)[N], bool& active, const std::string& savedValue)
        {
            if (!active)
                snprintf(buffer, N, "%s", savedValue.c_str());
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText(id, buffer, N);
            active = ImGui::IsItemActive();
            return ImGui::IsItemDeactivatedAfterEdit();
        }
    }

    int BuildSettingsWindow::counter_ = 0;

    BuildSettingsWindow::BuildSettingsWindow()
    {
        id_ = counter_++;
    }

    PopupWindowState BuildSettingsWindow::OnDraw(PopupWindowDrawGuiContext context)
    {
        if (focusRequested_)
        {
            ImGui::SetNextWindowFocus();
            focusRequested_ = false;
        }
        ImGui::SetNextWindowSize(ImVec2(620, 560), ImGuiCond_FirstUseEver);

        bool isOpen = true;
        if (ImGui::Begin(("Build Settings##" + std::to_string(id_)).c_str(), &isOpen))
        {
            OnDrawGameSettings();
            OnDrawBuildSettings();
            OnDrawActions();
            OnDrawLastBuild();
        }
        ImGui::End();

        return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
    }

    void BuildSettingsWindow::OnDrawGameSettings()
    {
        using Application::Configuration::BuildConfiguration;

        ImGui::SeparatorText("Game");

        BuildSettingsLabel("Product Name");
        if (BuildSettingsEditText("##BuildSettingsProductName", productName_.buffer, productName_.active, BuildConfiguration::ProductName()))
        {
            BuildConfiguration::SetProductName(productName_.buffer);
        }
        if (const std::string reason = BuildConfiguration::ValidateProductName(productName_.buffer); !reason.empty())
        {
            BuildSettingsErrorText(reason);
        }

        const std::string&                                     startSceneGuid = BuildConfiguration::StartSceneGuid();
        const std::vector<std::shared_ptr<Module::Asset::SceneFile>> sceneFiles = BuildConfiguration::CollectSceneFiles();
        std::shared_ptr<Module::Asset::SceneFile> startSceneFile;
        for (const auto& sceneFile : sceneFiles)
        {
            if (!startSceneGuid.empty() && sceneFile->GetGuid().Value() == startSceneGuid)
                startSceneFile = sceneFile;
        }

        const std::string defaultLabel = std::string("(Default) ") + BuildConfiguration::DefaultStartScenePath();
        std::string preview = defaultLabel;
        if (!startSceneGuid.empty())
            preview = startSceneFile ? startSceneFile->GetContentPath() : "(Missing) " + startSceneGuid;

        BuildSettingsLabel("Start Scene");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##BuildSettingsStartScene", preview.c_str()))
        {
            if (ImGui::Selectable(defaultLabel.c_str(), startSceneGuid.empty()))
            {
                BuildConfiguration::SetStartSceneGuid("");
            }
            for (const auto& sceneFile : sceneFiles)
            {
                const std::string& guid     = sceneFile->GetGuid().Value();
                const bool         selected = guid == startSceneGuid;
                ImGui::PushID(guid.c_str());
                if (ImGui::Selectable(sceneFile->GetContentPath().c_str(), selected))
                {
                    BuildConfiguration::SetStartSceneGuid(guid);
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        if (!startSceneGuid.empty() && !startSceneFile)
        {
            BuildSettingsErrorText("Start scene not found. Pick it again.");
        }
    }

    void BuildSettingsWindow::OnDrawBuildSettings()
    {
        using Application::Configuration::BuildConfiguration;
        using Application::Configuration::BuildTargetConfiguration;

        ImGui::SeparatorText("Build");

        BuildSettingsLabel("Configuration");
        const BuildTargetConfiguration targetConfiguration = BuildConfiguration::TargetConfiguration();
        if (ImGui::RadioButton("Release", targetConfiguration == BuildTargetConfiguration::Release))
        {
            BuildConfiguration::SetTargetConfiguration(BuildTargetConfiguration::Release);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Debug", targetConfiguration == BuildTargetConfiguration::Debug))
        {
            BuildConfiguration::SetTargetConfiguration(BuildTargetConfiguration::Debug);
        }

        BuildSettingsLabel("Output Directory");
        if (BuildSettingsEditText("##BuildSettingsOutputDirectory", outputDirectory_.buffer, outputDirectory_.active, BuildConfiguration::OutputDirectoryUtf8()))
        {
            BuildConfiguration::SetOutputDirectory(outputDirectory_.buffer);
        }
        ImGui::SetCursorPosX(BUILD_SETTINGS_LABEL_WIDTH);
        ImGui::TextDisabled("%s", BuildSettingsPathToUtf8(BuildConfiguration::OutputDirectory()).c_str());
        ImGui::SetCursorPosX(BUILD_SETTINGS_LABEL_WIDTH);
        if (ImGui::Button("Open Output Folder"))
        {
            const std::filesystem::path outputDirectory = BuildConfiguration::OutputDirectory();
            if (std::error_code ec; !std::filesystem::is_directory(outputDirectory, ec))
            {
                Module::LogWarning("BuildSettingsWindow: 出力先がまだありません: " + BuildSettingsPathToUtf8(outputDirectory));
            }
            else if (const HINSTANCE result = ShellExecuteW(nullptr, L"open", outputDirectory.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                     reinterpret_cast<INT_PTR>(result) <= 32)
            {
                Module::LogError("BuildSettingsWindow: 出力先を開けませんでした: " + BuildSettingsPathToUtf8(outputDirectory));
            }
        }

        BuildSettingsLabel("Asset Updates");
        if (bool assetUpdates = BuildConfiguration::AssetUpdatesEnabled(); ImGui::Checkbox("Write installed.json##BuildSettingsAssetUpdates", &assetUpdates))
        {
            BuildConfiguration::SetAssetUpdatesEnabled(assetUpdates);
        }
        ImGui::SetCursorPosX(BUILD_SETTINGS_LABEL_WIDTH);
        ImGui::TextDisabled("%s", BuildConfiguration::AssetUpdatesEnabled()
                                         ? "The game updates its Assets/ to the published manifest on the title screen"
                                         : "The game never checks for asset updates");

        BuildSettingsLabel("MSBuild");
        if (BuildSettingsEditText("##BuildSettingsMsBuildPath", msBuildPath_.buffer, msBuildPath_.active, BuildConfiguration::MsBuildPathUtf8()))
        {
            BuildConfiguration::SetMsBuildPath(msBuildPath_.buffer);
        }
        if (std::error_code ec; !std::filesystem::is_regular_file(BuildConfiguration::MsBuildPath(), ec))
        {
            BuildSettingsErrorText("MSBuild not found");
        }
        else if (BuildConfiguration::MsBuildPathUtf8().empty())
        {
            ImGui::TextDisabled("(auto) %s", BuildSettingsPathToUtf8(BuildConfiguration::MsBuildPath()).c_str());
        }
    }

    void BuildSettingsWindow::OnDrawActions()
    {
        using Application::Build::BuildAction;

        ImGui::Separator();

        auto& gameBuilder = Application::Build::GameBuilder::Instance();
        if (gameBuilder.IsBusy())
        {
            if (ImGui::Button("Cancel"))
            {
                gameBuilder.Cancel();
            }
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Building: %s %s", gameBuilder.PhaseLabel(), gameBuilder.ElapsedLabel().c_str());
            return;
        }

        if (ImGui::Button("Build"))
        {
            gameBuilder.Begin(BuildAction::Build);
        }
        ImGui::SameLine();
        if (ImGui::Button("Build And Run"))
        {
            gameBuilder.Begin(BuildAction::BuildAndRun);
        }
    }

    void BuildSettingsWindow::OnDrawLastBuild()
    {
        using Application::Build::BuildOutcome;

        ImGui::SeparatorText("Last Build");

        const auto& gameBuilder = Application::Build::GameBuilder::Instance();
        const auto  report      = gameBuilder.LastReport();
        switch (report.outcome)
        {
        case BuildOutcome::None:
            if (report.errors.empty())
            {
                ImGui::TextDisabled(gameBuilder.IsBusy() ? "In progress" : "Not built yet");
                return;
            }
            ImGui::TextDisabled("In progress");
            break;
        case BuildOutcome::Succeeded:
            ImGui::TextColored(BUILD_SETTINGS_SUCCESS_COLOR, "Succeeded (%s)", report.elapsed.c_str());
            ImGui::TextWrapped("%s", report.exePath.c_str());
            break;
        case BuildOutcome::Failed:
            ImGui::TextColored(BUILD_SETTINGS_ERROR_COLOR, "Failed (%s)", report.elapsed.c_str());
            break;
        case BuildOutcome::Canceled:
            ImGui::TextDisabled("Canceled (%s)", report.elapsed.c_str());
            break;
        }

        if (report.errors.empty())
            return;

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Errors (%zu)", report.errors.size() + report.omittedErrorCount);
        ImGui::SameLine();
        if (ImGui::SmallButton("Copy All"))
        {
            std::string allErrors;
            for (const auto& error : report.errors)
            {
                allErrors += error;
                allErrors += '\n';
            }
            ImGui::SetClipboardText(allErrors.c_str());
        }

        ImGui::BeginChild("##BuildSettingsErrors", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, BUILD_SETTINGS_ERROR_COLOR);
        for (size_t i = 0; i < report.errors.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(report.errors[i].c_str()))
            {
                ImGui::SetClipboardText(report.errors[i].c_str());
            }
            ImGui::PopID();
        }
        ImGui::PopStyleColor();
        if (report.omittedErrorCount > 0)
        {
            ImGui::TextDisabled("... %zu more lines in GameBuild.errors.log (staging)", report.omittedErrorCount);
        }
        ImGui::EndChild();
    }
}

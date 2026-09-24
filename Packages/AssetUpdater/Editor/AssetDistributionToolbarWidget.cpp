#include "AssetDistributionToolbarWidget.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

#include "ImGuiHelper.h"
#include "../Text/Utf8.h"
#include "../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../Engine/Core/Application/Configuration/Build/ApplicationConfiguration_Build.h"
#include "../../../Engine/Core/Application/Window/Toolbar/Widget/EditorToolbarWidgetRegistry.h"
#include "../../../Engine/Module/Exception/Engine_Module_Exception.h"
#include "../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../../Engine/Module/ProjectConfig/Engine_Module_ProjectConfig.h"

namespace NanamiEngine::AssetUpdater::Editor
{
    namespace
    {
        constexpr auto ASSET_DIST_CONFIG_PATH                 = "Build/AssetDistribution/";
        constexpr auto ASSET_DIST_VERSION_KEY                 = "Version";
        constexpr auto ASSET_DIST_REQUIRED_CLIENT_VERSION_KEY = "RequiredClientVersion";
        constexpr auto ASSET_DIST_PYTHON_KEY                  = "Python";
        constexpr auto ASSET_DIST_DEFAULT_PYTHON              = "python";

        constexpr auto ASSET_DIST_TOOL_ENTRY  = "tools/dist/__main__.py";
        constexpr auto ASSET_DIST_POPUP       = "AssetDistributionPopup";
        constexpr auto ASSET_DIST_CONFIRM     = "Release assets?##AssetDistribution";
        constexpr size_t ASSET_DIST_MAX_LINES = 5000;

        // tools/dist/upload.py の VERSION_RE と揃える
        bool AssetDistIsValidVersion(const std::string& version)
        {
            return !version.empty() && std::ranges::all_of(version, [](const char c)
            {
                return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '.' || c == '_' || c == '-';
            });
        }

        template <size_t N>
        void AssetDistCopyToBuffer(char (&buffer)[N], const std::string& value)
        {
            strncpy_s(buffer, value.c_str(), _TRUNCATE);
        }
    }

    bool AssetDistributionToolbarWidget::IsVisible() const
    {
        if constexpr (Core::Application::Configuration::APPLICATION_MODE != Core::Application::Configuration::ApplicationMode::Editor)
        {
            return false;
        }
        else
        {
            if (!toolsAvailable_)
            {
                std::error_code ec;
                toolsAvailable_ = std::filesystem::is_regular_file(ASSET_DIST_TOOL_ENTRY, ec);
            }
            return *toolsAvailable_;
        }
    }

    void AssetDistributionToolbarWidget::OnDraw(Core::Toolbar::EditorToolbarWidgetContext&)
    {
        if (!settingsLoaded_)
            LoadSettings();
        PollFinished();

        if (ImGui::Button("Asset Dist"))
        {
            ImGui::OpenPopup(ASSET_DIST_POPUP);
        }

        // ポップアップを閉じていても進み具合が分かるよう、実行中はツールバーにも出す
        if (process_.IsRunning())
        {
            ImGui::SameLine();
            ImGui::Text("Dist: %s %s", StepLabel(runningStep_), process_.ElapsedLabel().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Cancel Dist"))
            {
                process_.Cancel();
            }
        }

        if (ImGui::BeginPopup(ASSET_DIST_POPUP))
        {
            OnDrawPopup();
            ImGui::EndPopup();
        }
    }

    void AssetDistributionToolbarWidget::LoadSettings()
    {
        using namespace Module::ProjectConfig;
        AssetDistCopyToBuffer(version_,               LoadOrDefaultWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_VERSION_KEY,                 std::string()));
        AssetDistCopyToBuffer(requiredClientVersion_, LoadOrDefaultWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_REQUIRED_CLIENT_VERSION_KEY, std::string()));
        AssetDistCopyToBuffer(python_,                LoadOrDefaultWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_PYTHON_KEY,                  std::string(ASSET_DIST_DEFAULT_PYTHON)));
        settingsLoaded_ = true;
    }

    void AssetDistributionToolbarWidget::SaveSettings() const
    {
        try
        {
            using namespace Module::ProjectConfig;
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_VERSION_KEY,                 version_);
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_REQUIRED_CLIENT_VERSION_KEY, requiredClientVersion_);
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_PYTHON_KEY,                  python_);
        }
        catch (const Module::Exception::NanamiException& exception)
        {
            Module::LogError("AssetDistribution: 設定を保存できませんでした: " + std::string(exception.what()));
        }
    }

    void AssetDistributionToolbarWidget::OnDrawPopup()
    {
        const bool running = process_.IsRunning();

        ImGui::TextUnformatted("Asset Distribution (tools/dist -> Cloudflare R2)");
        ImGui::Separator();

        ImGui::BeginDisabled(running);
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("Version", version_, sizeof(version_));
        if (ImGui::IsItemDeactivatedAfterEdit())
            SaveSettings();
        ImGui::SetNextItemWidth(200);
        // NOTE: 空なら tools/dist が Build Settings の Client Version を使う
        const std::string clientVersionHint = "Build Settings: " + Core::Application::Configuration::BuildConfiguration::ClientVersion();
        ImGui::InputTextWithHint("Required Client Version", clientVersionHint.c_str(), requiredClientVersion_, sizeof(requiredClientVersion_));
        if (ImGui::IsItemDeactivatedAfterEdit())
            SaveSettings();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Games older than this skip the update. Raise it only when shipping a new exe (e.g. font changes)");
        ImGui::SetNextItemWidth(400);
        ImGui::InputText("Python", python_, sizeof(python_));
        if (ImGui::IsItemDeactivatedAfterEdit())
            SaveSettings();
        ImGui::EndDisabled();

        const std::string inputError = ValidateInputs();
        if (!inputError.empty())
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", inputError.c_str());

        ImGui::Spacing();
        if (running)
        {
            ImGui::Text("Running: %s %s", StepLabel(runningStep_), process_.ElapsedLabel().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                process_.Cancel();
        }
        else
        {
            ImGui::BeginDisabled(!inputError.empty());
            if (ImGui::Button("Build Manifest"))
                Begin(Step::Build);
            ImGui::SameLine();
            if (ImGui::Button("Upload (Dry Run)"))
                Begin(Step::DryRun);
            ImGui::SameLine();
            const bool built = builtVersion_ == version_;
            ImGui::BeginDisabled(!built);
            if (ImGui::Button("Upload (Release)"))
                ImGui::OpenPopup(ASSET_DIST_CONFIRM);
            ImGui::EndDisabled();
            if (!built && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("Build Manifest for this version first");
            ImGui::EndDisabled();
        }
        OnDrawReleaseConfirm();

        if (lastStep_ != Step::None && !running)
        {
            if (lastCanceled_)
                ImGui::Text("Last: %s canceled (%s)", StepLabel(lastStep_), lastElapsed_.c_str());
            else if (lastExitCode_ == 0)
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Last: %s succeeded (%s)", StepLabel(lastStep_), lastElapsed_.c_str());
            else
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Last: %s failed (exit %d, %s)", StepLabel(lastStep_), lastExitCode_.value_or(-1), lastElapsed_.c_str());
        }

        ImGui::Separator();
        OnDrawLog();
    }

    void AssetDistributionToolbarWidget::OnDrawReleaseConfirm()
    {
        if (!ImGui::BeginPopupModal(ASSET_DIST_CONFIRM, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::Text("Release version %s to every player?", version_);
        ImGui::TextUnformatted("manifest.json on R2 will be replaced. Clients download it on their next start.");
        ImGui::Spacing();
        if (ImGui::Button("Release"))
        {
            Begin(Step::Release);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    void AssetDistributionToolbarWidget::OnDrawLog()
    {
        const size_t total = process_.CopyLines(logLines_, copiedLineCount_);
        if (total != copiedLineCount_)
        {
            copiedLineCount_ = total;
            scrollToBottom_  = true;
        }
        if (logLines_.size() > ASSET_DIST_MAX_LINES)
            logLines_.erase(logLines_.begin(), logLines_.begin() + static_cast<std::ptrdiff_t>(logLines_.size() - ASSET_DIST_MAX_LINES));

        if (ImGui::Button("Copy Log"))
        {
            std::string text;
            for (const auto& line : logLines_)
                text += line + "\n";
            ImGui::SetClipboardText(text.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Log"))
            logLines_.clear();

        ImGui::BeginChild("AssetDistributionLog", ImVec2(720, 320), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& line : logLines_)
            ImGui::TextUnformatted(line.c_str());
        if (scrollToBottom_)
        {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom_ = false;
        }
        ImGui::EndChild();
    }

    void AssetDistributionToolbarWidget::Begin(const Step step)
    {
        if (process_.IsRunning() || !ValidateInputs().empty())
            return;
        SaveSettings();

        std::wstring commandLine = L"\"" + Utf8ToWide(python_) + L"\" -X utf8 -u -m tools.dist ";
        switch (step)
        {
        case Step::Build:
            commandLine += L"build --version " + Utf8ToWide(version_);
            if (requiredClientVersion_[0] != '\0')
                commandLine += L" --required-client-version " + Utf8ToWide(requiredClientVersion_);
            break;
        case Step::DryRun:
            commandLine += L"upload --dry-run";
            break;
        case Step::Release:
            commandLine += L"upload";
            break;
        case Step::None:
            return;
        }

        logLines_.clear();
        copiedLineCount_ = 0;
        runningVersion_  = version_;
        runningStep_     = step;
        if (!process_.Start(commandLine, std::filesystem::current_path()))
        {
            // NOTE: 起動の失敗理由は process_ の出力に入っている
            lastStep_     = step;
            lastExitCode_.reset();
            lastCanceled_ = false;
            lastElapsed_  = process_.ElapsedLabel();
            runningStep_  = Step::None;
            Module::LogError(std::string("AssetDistribution: ") + StepLabel(step) + " を起動できませんでした");
            return;
        }
        Module::Log(std::string("AssetDistribution: ") + StepLabel(step) + " を始めました (" + runningVersion_ + ")");
    }

    void AssetDistributionToolbarWidget::PollFinished()
    {
        if (runningStep_ == Step::None || process_.IsRunning())
            return;

        lastStep_     = runningStep_;
        lastExitCode_ = process_.ExitCode();
        lastCanceled_ = process_.WasCanceled();
        lastElapsed_  = process_.ElapsedLabel();
        runningStep_  = Step::None;

        const std::string label = std::string("AssetDistribution: ") + StepLabel(lastStep_);
        if (lastCanceled_)
        {
            Module::Log(label + " を中止しました");
        }
        else if (lastExitCode_ == 0)
        {
            if (lastStep_ == Step::Build)
                builtVersion_ = runningVersion_;
            Module::Log(label + " が完了しました (" + runningVersion_ + ", " + lastElapsed_ + ")");
        }
        else
        {
            Module::LogError(label + " に失敗しました (exit " + std::to_string(lastExitCode_.value_or(-1)) + ")。Asset Dist のログを確認してください");
        }
    }

    std::string AssetDistributionToolbarWidget::ValidateInputs() const
    {
        if (!AssetDistIsValidVersion(version_))
            return "Version must be [0-9A-Za-z._-]+";
        if (requiredClientVersion_[0] != '\0' && !AssetDistIsValidVersion(requiredClientVersion_))
            return "Required Client Version must be [0-9A-Za-z._-]+";
        if (python_[0] == '\0' || std::strchr(python_, '"'))
            return "Python must be a path without \"";
        return {};
    }

    const char* AssetDistributionToolbarWidget::StepLabel(const Step step)
    {
        switch (step)
        {
        case Step::Build:   return "build";
        case Step::DryRun:  return "dry run";
        case Step::Release: return "release";
        case Step::None:    break;
        }
        return "";
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(AssetDistributionToolbarWidget, 350)
}

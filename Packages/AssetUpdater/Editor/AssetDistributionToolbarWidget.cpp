#include "AssetDistributionToolbarWidget.h"

#include <utility>
#include <cstring>
#include <filesystem>

#include "ImGuiHelper.h"
#include "../Text/VersionString.h"
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
        constexpr auto ASSET_DIST_REMOTE_KEY                  = "Remote";
        constexpr auto ASSET_DIST_PUBLIC_BASE_URL_KEY         = "PublicBaseUrl";
        constexpr auto ASSET_DIST_RCLONE_KEY                  = "Rclone";
        constexpr auto ASSET_DIST_DEFAULT_RCLONE              = "rclone";

        constexpr auto ASSET_DIST_CONFIG_DIRECTORY = "ProjectConfig/Build/AssetDistribution";
        constexpr auto ASSET_DIST_POPUP            = "AssetDistributionPopup";
        constexpr auto ASSET_DIST_CONFIRM          = "Release assets?##AssetDistribution";
        constexpr size_t ASSET_DIST_MAX_LINES      = 5000;

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
            if (!configAvailable_)
            {
                std::error_code ec;
                configAvailable_ = std::filesystem::is_directory(ASSET_DIST_CONFIG_DIRECTORY, ec);
            }
            return *configAvailable_;
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
        if (job_.IsRunning())
        {
            ImGui::SameLine();
            ImGui::Text("Dist: %s %s", StepLabel(runningStep_), job_.ElapsedLabel().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Cancel Dist"))
            {
                job_.Cancel();
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
        AssetDistCopyToBuffer(remote_,                LoadOrDefaultWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_REMOTE_KEY,                  std::string()));
        AssetDistCopyToBuffer(publicBaseUrl_,         LoadOrDefaultWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_PUBLIC_BASE_URL_KEY,         std::string()));
        AssetDistCopyToBuffer(rclone_,                LoadOrDefaultWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_RCLONE_KEY,                  std::string(ASSET_DIST_DEFAULT_RCLONE)));
        settingsLoaded_ = true;
    }

    void AssetDistributionToolbarWidget::SaveSettings() const
    {
        try
        {
            using namespace Module::ProjectConfig;
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_VERSION_KEY,                 version_);
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_REQUIRED_CLIENT_VERSION_KEY, requiredClientVersion_);
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_REMOTE_KEY,                  remote_);
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_PUBLIC_BASE_URL_KEY,         publicBaseUrl_);
            SaveWithPath<std::string>(ASSET_DIST_CONFIG_PATH, ASSET_DIST_RCLONE_KEY,                  rclone_);
        }
        catch (const Module::Exception::NanamiException& exception)
        {
            Module::LogError("AssetDistribution: 設定を保存できませんでした: " + std::string(exception.what()));
        }
    }

    void AssetDistributionToolbarWidget::OnDrawPopup()
    {
        const bool running = job_.IsRunning();

        ImGui::TextUnformatted("Asset Distribution (manifest.json -> Cloudflare R2 via rclone)");
        ImGui::Separator();

        ImGui::BeginDisabled(running);
        OnDrawSettings();
        ImGui::EndDisabled();

        const std::string inputError = ValidateInputs();
        if (!inputError.empty())
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", inputError.c_str());

        ImGui::Spacing();
        if (running)
        {
            ImGui::Text("Running: %s %s", StepLabel(runningStep_), job_.ElapsedLabel().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                job_.Cancel();
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
            ImGui::SameLine();
            if (ImGui::Button("Diff vs Live"))
                Begin(Step::DiffLive);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("What players would download: the live manifest.json on R2 vs the local one");
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Self Test"))
                Begin(Step::SelfTest);
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

    void AssetDistributionToolbarWidget::OnDrawSettings()
    {
        const auto input = [this](const char* label, char* buffer, const size_t size, const float width)
        {
            ImGui::SetNextItemWidth(width);
            ImGui::InputText(label, buffer, size);
            if (ImGui::IsItemDeactivatedAfterEdit())
                SaveSettings();
        };

        input("Version", version_, sizeof(version_), 200);
        ImGui::SetNextItemWidth(200);
        const std::string clientVersionHint = "Build Settings: " + Core::Application::Configuration::BuildConfiguration::ClientVersion();
        ImGui::InputTextWithHint("Required Client Version", clientVersionHint.c_str(), requiredClientVersion_, sizeof(requiredClientVersion_));
        if (ImGui::IsItemDeactivatedAfterEdit())
            SaveSettings();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Games older than this skip the update. Raise it only when shipping a new exe (e.g. font changes)");

        if (ImGui::TreeNode("Destination##AssetDistribution"))
        {
            input("Remote", remote_, sizeof(remote_), 400);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("rclone remote and bucket (e.g. r2:nanami-assets). Keys stay in rclone.conf");
            input("Public Base URL", publicBaseUrl_, sizeof(publicBaseUrl_), 400);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("URL players read from. baseUrl = <this>/files/, manifest = <this>/manifest.json");
            input("Rclone", rclone_, sizeof(rclone_), 400);
            ImGui::TreePop();
        }
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
        const size_t total = job_.CopyLines(logLines_, copiedLineCount_);
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
        if (job_.IsRunning() || step == Step::None)
            return;
        if (step != Step::SelfTest && !ValidateInputs().empty())
            return;
        SaveSettings();

        Dist::DistRequest request;
        request.step                  = step;
        request.version               = version_;
        request.requiredClientVersion = RequiredClientVersion();
        request.config.remote         = remote_;
        request.config.publicBaseUrl  = publicBaseUrl_;
        request.config.rclone         = rclone_;
        request.repoRoot              = std::filesystem::current_path();

        logLines_.clear();
        copiedLineCount_ = 0;
        runningVersion_  = version_;
        runningStep_     = step;
        job_.Start(std::move(request));
        Module::Log(std::string("AssetDistribution: ") + StepLabel(step) + " を始めました (" + runningVersion_ + ")");
    }

    void AssetDistributionToolbarWidget::PollFinished()
    {
        if (runningStep_ == Step::None || job_.IsRunning())
            return;

        lastStep_     = runningStep_;
        lastExitCode_ = job_.ExitCode();
        lastCanceled_ = job_.WasCanceled();
        lastElapsed_  = job_.ElapsedLabel();
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
            Module::LogError(label + " に失敗しました。Asset Dist のログを確認してください");
        }
    }

    std::string AssetDistributionToolbarWidget::ValidateInputs() const
    {
        if (!IsValidVersionString(version_))
            return "Version must be [0-9A-Za-z._-]+";
        if (!IsValidVersionString(RequiredClientVersion()))
            return "Required Client Version must be [0-9A-Za-z._-]+";
        if (rclone_[0] == '\0')
            return "Rclone must be a path (or rclone on PATH)";
        return {};
    }

    std::string AssetDistributionToolbarWidget::RequiredClientVersion() const
    {
        // NOTE: 空なら Build Settings の Client Version
        return requiredClientVersion_[0] != '\0' ? std::string(requiredClientVersion_) : Core::Application::Configuration::BuildConfiguration::ClientVersion();
    }

    const char* AssetDistributionToolbarWidget::StepLabel(const Step step)
    {
        switch (step)
        {
        case Step::Build:    return "build";
        case Step::DryRun:   return "dry run";
        case Step::Release:  return "release";
        case Step::DiffLive: return "diff vs live";
        case Step::SelfTest: return "self test";
        case Step::None:     break;
        }
        return "";
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(AssetDistributionToolbarWidget, 350)
}

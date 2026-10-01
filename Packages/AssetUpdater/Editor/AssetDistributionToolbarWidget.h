#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "../Dist/DistJob.h"
#include "../../../Engine/Core/Application/Window/Toolbar/Widget/IEditorToolbarWidget.h"

namespace NanamiEngine::AssetUpdater::Editor
{
    class NANAMI_API AssetDistributionToolbarWidget final : public Core::Toolbar::IEditorToolbarWidget
    {
    public:
        [[nodiscard]] bool IsVisible() const override;
        void OnDraw(Core::Toolbar::EditorToolbarWidgetContext& context) override;

    private:
        using Step = Dist::DistStep;

        void LoadSettings();
        void SaveSettings() const;
        void OnDrawPopup();
        void OnDrawSettings();
        void OnDrawReleaseConfirm();
        void OnDrawLog();
        void Begin(Step step);
        void PollFinished();
        [[nodiscard]] std::string ValidateInputs() const;
        [[nodiscard]] std::string RequiredClientVersion() const;

        [[nodiscard]] static const char* StepLabel(Step step);

        mutable std::optional<bool> configAvailable_;
        bool settingsLoaded_ = false;
        char version_              [64]  = {};
        char requiredClientVersion_[64]  = {};
        char remote_               [260] = {};
        char publicBaseUrl_        [260] = {};
        char rclone_               [260] = {};

        Dist::DistJob            job_;
        Step                     runningStep_ = Step::None;
        Step                     lastStep_    = Step::None;
        std::string              runningVersion_;
        
        std::string              builtVersion_;
        std::optional<int>       lastExitCode_;
        bool                     lastCanceled_ = false;
        std::string              lastElapsed_;
        std::vector<std::string> logLines_;
        size_t                   copiedLineCount_ = 0;
        bool                     scrollToBottom_  = false;
    };
}

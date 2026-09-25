#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "../../../Engine/Core/Application/Process/AsyncProcess.h"
#include "../../../Engine/Core/Application/Window/Toolbar/Widget/IEditorToolbarWidget.h"

namespace NanamiEngine::AssetUpdater::Editor
{
    /** @brief tools/dist (manifest の build と R2 への upload) をエディタから動かす。tools/dist が無いプロジェクトでは出さない */
    class NANAMI_API AssetDistributionToolbarWidget final : public Core::Toolbar::IEditorToolbarWidget
    {
    public:
        [[nodiscard]] bool IsVisible() const override;
        void OnDraw(Core::Toolbar::EditorToolbarWidgetContext& context) override;

    private:
        enum class Step
        {
            None,
            Build,
            DryRun,
            Release,
        };

        void LoadSettings();
        void SaveSettings() const;
        void OnDrawPopup();
        void OnDrawReleaseConfirm();
        void OnDrawLog();
        void Begin(Step step);
        void PollFinished();
        [[nodiscard]] std::string ValidateInputs() const;

        [[nodiscard]] static const char* StepLabel(Step step);

        // NOTE: 作業ディレクトリ (-project) が決まった後に一度だけ調べる
        mutable std::optional<bool> toolsAvailable_;
        bool settingsLoaded_ = false;
        char version_              [64]  = {};
        char requiredClientVersion_[64]  = {};
        char python_               [260] = {};

        Core::Application::Process::AsyncProcess process_;
        Step                     runningStep_ = Step::None;
        Step                     lastStep_    = Step::None;
        std::string              runningVersion_;
        /** @brief このセッションで build に成功した版。Release はこれと Version が一致するときだけ押せる */
        std::string              builtVersion_;
        std::optional<int>       lastExitCode_;
        bool                     lastCanceled_ = false;
        std::string              lastElapsed_;
        std::vector<std::string> logLines_;
        size_t                   copiedLineCount_ = 0;
        bool                     scrollToBottom_  = false;
    };
}

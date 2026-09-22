#pragma once
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"

namespace GamePlay::Ui
{
    class SwordManActionInstructTutorial;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class IControlGuideFocusRequest;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class ActionInstructTutorialModel;

    /// 課題を順に出し、達成イベントを待ち、操作ガイドに「次に押す操作」を指させる
    class ActionInstructTutorialPresenter final
    {
    public:
        ActionInstructTutorialPresenter(
            std::unique_ptr<ActionInstructTutorialModel> model,
            IControlGuideFocusRequest& guideFocus,
            const std::weak_ptr<GamePlay::Ui::SwordManActionInstructTutorial>& view);
        ~ActionInstructTutorialPresenter();

        Coroutine::Task<void> SubscribeModelEventToViewAsync();

    private:
        std::unique_ptr<ActionInstructTutorialModel> model_;
        IControlGuideFocusRequest& guideFocus_;
        std::weak_ptr<GamePlay::Ui::SwordManActionInstructTutorial> view_;
    };
}

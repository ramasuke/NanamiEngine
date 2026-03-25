#pragma once
#include <memory>

#include "../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"

namespace GamePlay::Ui
{
    class SwordManActionInstructTutorial;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class ActionInstructTutorialModel;
}

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    class ActionInstructTutorialPresenter final
    {
    public:
        explicit ActionInstructTutorialPresenter(
            std::unique_ptr<ActionInstructTutorialModel> model,
            const std::weak_ptr<GamePlay::Ui::SwordManActionInstructTutorial>& view);
        ~ActionInstructTutorialPresenter();
        Coroutine::Task<void> SubscribeModelEventToViewAsync();
        
        
    private:
        std::unique_ptr<ActionInstructTutorialModel> model_;
        std::weak_ptr<GamePlay::Ui::SwordManActionInstructTutorial> view_;
    };
}

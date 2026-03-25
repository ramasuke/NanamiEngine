#include "Quest_SwordMan_ActionInstructTutorialPresenter.h"

#include "Quest_SwordMan_ActionInstructTutorialModel.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForObservable/WaitForObservable.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../GamePlay/Ui/ActionInstructTutorial/SwordMan/Ui_SwordMan_ActionInstructTutorial.h"
#include "../../../Event/IObservableStatusEvent.h"


namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    ActionInstructTutorialPresenter::ActionInstructTutorialPresenter(
        std::unique_ptr<ActionInstructTutorialModel> model,
        const std::weak_ptr<GamePlay::Ui::SwordManActionInstructTutorial>& view )
        : model_(std::move(model))
        , view_ (view)
    {
        
    }
    ActionInstructTutorialPresenter::~ActionInstructTutorialPresenter() = default;


    Coroutine::Task<void> ActionInstructTutorialPresenter::SubscribeModelEventToViewAsync()
    {
        view_.lock()->OnDisplayAttackText();
        co_await Coroutine::WaitForObservable(model_->StatusEvent().OnComboAttack());

        view_.lock()->OnDisplayRunText();
        co_await Coroutine::WaitForObservable(model_->StatusEvent().OnRun());

        view_.lock()->OnDisplayDashAttackText();
        co_await Coroutine::WaitForObservable(model_->StatusEvent().OnDashAttack());
        
        view_.lock()->Hide();
    }
}

#include "Quest_SwordMan_ActionInstructTutorialPresenter.h"

#include <array>

#include "Quest_SwordMan_ActionInstructTutorialModel.h"
#include "Packages/R4/R4.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForObservable/Coroutine_WaitForObservable.h"
#include "Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../GamePlay/Ui/ActionInstructTutorial/SwordMan/Ui_SwordMan_ActionInstructTutorial.h"
#include "../../../ControlGuideFocus/SwordMan_IControlGuideFocusRequest.h"
#include "../../../Event/IObservableStatusEvent.h"

namespace GameCore::PlayerAvatar::SwordMan::Quest
{
    namespace
    {
        /// 課題の順番。ガイドのどの行を指すかと、何が起きたら達成かをここだけで決める
        struct ActionInstructTutorialStepPlan
        {
            SwordManControlGuideFocus focus;
            R4::Observable<R4::Unit> (IObservableStatusEvent::*completed)() const;
        };

        constexpr std::array ACTION_INSTRUCT_STEPS = {
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::Move,         &IObservableStatusEvent::OnMove         },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::Jump,         &IObservableStatusEvent::OnJump         },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::Attack,       &IObservableStatusEvent::OnComboAttack  },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::ChargeAttack, &IObservableStatusEvent::OnChargeAttack },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::Run,          &IObservableStatusEvent::OnRun          },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::Attack,       &IObservableStatusEvent::OnDashAttack   },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::AvoidRolling, &IObservableStatusEvent::OnAvoidRolling },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::Attack,       &IObservableStatusEvent::OnJumpAttack   },
            ActionInstructTutorialStepPlan{ SwordManControlGuideFocus::LockOn,       &IObservableStatusEvent::OnLockOn       },
        };
    }

    ActionInstructTutorialPresenter::ActionInstructTutorialPresenter(
        std::unique_ptr<ActionInstructTutorialModel> model,
        IControlGuideFocusRequest& guideFocus,
        const std::weak_ptr<GamePlay::Ui::SwordManActionInstructTutorial>& view)
        : model_     (std::move(model))
        , guideFocus_(guideFocus)
        , view_      (view)
    {

    }
    ActionInstructTutorialPresenter::~ActionInstructTutorialPresenter() = default;

    Coroutine::Task<void> ActionInstructTutorialPresenter::SubscribeModelEventToViewAsync()
    {
        for (std::size_t i = 0; i < ACTION_INSTRUCT_STEPS.size(); ++i)
        {
            const auto view = view_.lock();
            if (!view)
                co_return;

            view->ShowStep(i);
            guideFocus_.SetFocus(ACTION_INSTRUCT_STEPS[i].focus);

            co_await Coroutine::WaitForObservable((model_->StatusEvent().*ACTION_INSTRUCT_STEPS[i].completed)());

            guideFocus_.MarkCleared();
            co_await view->PlayClearedAsync();
        }

        guideFocus_.ClearFocus();
        if (const auto view = view_.lock())
            view->Hide();
    }
}

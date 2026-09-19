#include "Enemy_Behaviour_Action_Stun.h"

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::Stun::DoTick(const TickContext& context)
    {
        const auto stunState = context.Parameter()->Catch<int>(stunStateKeyName_);
        if (!stunState || stunState->Get() == 0)
            return TickStatus::Failure;

        during_secs_ += Time::DeltaTime();

        const bool isDown = during_secs_ < downDuration_secs_;
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(isDown ? downAnimationNumber_ : getUpAnimationNumber_);

        if (during_secs_ < downDuration_secs_ + getUpDuration_secs_)
            return TickStatus::Running;

        during_secs_ = 0.0f;
        stunState->Set(0);
        return TickStatus::Success;
    }

    void Action::Stun::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stunStateKeyName_", stunStateKeyName_);
        ImGuiHelper::OnDrawInputField("downAnimationNumber_", downAnimationNumber_);
        ImGuiHelper::OnDrawInputField("getUpAnimationNumber_", getUpAnimationNumber_);
        ImGuiHelper::OnDrawInputField("downDuration_secs_", downDuration_secs_);
        ImGuiHelper::OnDrawInputField("getUpDuration_secs_", getUpDuration_secs_);
    }
}

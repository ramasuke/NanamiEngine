#include "Enemy_Behaviour_Action_PlayAnimation.h"

#include "../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlayAnimation::DoTick(const TickContext& context)
    {
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animatorSetParamNumber_);
        if (animationSound_)
        {
            GamePlay::Sound::SoundPlayer::PlaySe(*animationSound_.get(), context.EnemyTransform().GetWorldPos());
        }
        
        return TickStatus::Success;
    }

    void Action::PlayAnimation::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animatorSetParamNumber", animatorSetParamNumber_);
        ImGuiHelper::OnDrawInputField("animationSound_", animationSound_);
    }
}

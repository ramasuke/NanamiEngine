#include "SwordManAvatarWalkState.h"

#include "../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarWalkState::DoEnter()
    {

    }

    void SwordManAvatarWalkState::DoFixedUpdate()
    {
        const auto inputMove = Input().Move().ReadValue();
        Actions().ForwardMove(Status().GetWalkSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
    }

    void SwordManAvatarWalkState::DoUpdate()
    {
        TryEmitFootstep(Resources().WalkFootstepContactPhases(), Resources().WalkFootstepSounds());

        if (Status().IsInjured())
            OnChangeState(SwordManAvatarStateType::InjuredWalk);
        if (Status().IsDamaged())
            OnChangeState(SwordManAvatarStateType::Hurt);
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (Input().Run().IsUpdatePressed() && Status().CanRun())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run);
        if (Input().Jump().IsPressed())
            OnChangeState(SwordManAvatarStateType::Jump);
        if (Input().AvoidRolling().IsPressed())
            OnChangeState(SwordManAvatarStateType::AvoidRolling);
        if (Input().NormalAttack().IsPressed())
            OnChangeState(SwordManAvatarStateType::NormalAttack);
        if (Conditions().CanUseCannon())
            OnChangeState(SwordManAvatarStateType::UseCanon);
        if (!Conditions().IsGround())
            OnChangeState(SwordManAvatarStateType::Floating);
    }

    void SwordManAvatarWalkState::DoExit()
    {

    }
}

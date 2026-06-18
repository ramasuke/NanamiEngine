#include "SwordManAvatarWalkState.h"

#include "../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
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
        if (Status().IsDamaged())
            OnChangeState(SwordManAvatarStateType::Hurt);
        if (Status().IsOnDisableReinforceMode())
            OnChangeState(SwordManAvatarStateType::OnDisableReinforce);
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (Input().Run().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Run);
        if (Input().Jump().IsPressed())
            OnChangeState(SwordManAvatarStateType::Jump);
        if (Input().AvoidRolling().IsPressed())
            OnChangeState(SwordManAvatarStateType::AvoidRolling);
        if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
            OnChangeState(SwordManAvatarStateType::OnEnableReinforce);
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

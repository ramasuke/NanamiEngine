#pragma once
#include "../../StateMachine/PlayerAvatarStateMachine.h"
#include "ArmStretch/SwordManAvatarArmStretchState.h"
#include "Attack/Dash/SwordManAvatarDashAttackState.h"
#include "Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "Chatting/SwordManAvatarChattingState.h"
#include "ClimbToTop/SwordManAvatarStateClimbToTop.h"
#include "Death/SwordManAvatar_DeathState.h"
#include "DisableState/SwordManAvatar_DisableState.h"
#include "Floating/FloatingState.h"
#include "Hurt/SwordManAvatar_HurtState.h"
#include "Idle/SwordManAvatarIdleState.h"
#include "Jump/SwordManAvatarJumpState.h"
#include "OnDisableReinforce/OnDisableReinforceState.h"
#include "OnEnableReinforce/OnEnableReinforceState.h"
#include "Run/SwordManAvatarRunState.h"
#include "UseCanon/SwordManAvatarUseCanonState.h"
#include "Walk/SwordManAvatarWalkState.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    using SwordManAvatarStateMachine = PlayerAvatarStateMachine<  SwordManAvatarStateBase
                                                                , State::DisableState 
                                                                , State::SwordManAvatarIdleState
                                                                , State::SwordManAvatarWalkState
                                                                , State::SwordManAvatarRunState
                                                                , State::SwordManAvatarJumpState
                                                                , State::FloatingState
                                                                , State::SwordManAvatarNormalAttackState
                                                                , State::SwordManAvatarDashAttackState
                                                                , State::SwordManAvatarStateClimbToTop
                                                                , State::SwordManAvatarArmStretchState
                                                                , State::SwordManAvatarChattingState
                                                                , State::OnEnableReinforceState
                                                                , State::OnDisableReinforceState
                                                                , State::HurtState
                                                                , State::AvoidRollingState
                                                                , State::DeathState
                                                                , State::SwordManAvatarUseCannonState>;

    std::unique_ptr<SwordManAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup);
}

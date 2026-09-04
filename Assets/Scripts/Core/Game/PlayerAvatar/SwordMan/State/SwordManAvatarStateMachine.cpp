#include "SwordManAvatarStateMachine.h"

#include "ArmStretch/SwordManAvatarArmStretchState.h"
#include "Attack/Dash/SwordManAvatarDashAttackState.h"
#include "Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "AttackedShocked/SwordManAvatar_AttackedShockedState.h"
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
#include "InjuredRun/SwordManAvatarInjuredRunState.h"
#include "InjuredWalk/SwordManAvatarInjuredWalkState.h"
#include "Run/SwordManAvatarRunState.h"
#include "UseCanon/SwordManAvatarUseCanonState.h"
#include "Walk/SwordManAvatarWalkState.h"

#include "../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStateMachine::SwordManAvatarStateMachine(
        StatesFactory factory,
        const SwordManAvatarStateType initialState,
        const SwordManAvatarStateType disableState,
        const bool isEnable)
        : PlayerAvatarStateMachineBase(std::move(factory), initialState, disableState, isEnable)
        , swordManCurrentState_(nullptr)
    {
        PlayerAvatarStateMachineBase::CurrentState()
            .subscribe([this](const std::shared_ptr<IPlayerAvatarState>& state)
            {
                swordManCurrentState_.get_subscriber().on_next(
                    std::dynamic_pointer_cast<SwordManAvatarStateBase>(state));
            });
    }

    void SwordManAvatarStateMachine::OnChangeState(SwordManAvatarStateType type) { Base::OnChangeState(type); }
    void SwordManAvatarStateMachine::OnEnable()  { Base::OnEnable();  }
    void SwordManAvatarStateMachine::OnDisable() { Base::OnDisable(); }

    rxcpp::observable<std::shared_ptr<SwordManAvatarStateBase>> SwordManAvatarStateMachine::CurrentState() const
    {
        return swordManCurrentState_.get_observable();
    }

    std::unique_ptr<SwordManAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup
        , const bool isEnable)
    {
        auto context = std::make_shared<SwordManAvatarStateContext>(
            status,
            input,
            playerAvatar->Entity(),
            cameraGroup,
            playerAvatar->CatchNormalAttackArea(),
            playerAvatar->CatchDashAttackArea(),
            playerAvatar->Resources()
        );

        return std::make_unique<SwordManAvatarStateMachine>(
            [context](SwordManAvatarStateMachine::OnChangeStateCallback callback)
                -> SwordManAvatarStateMachine::StateMap
            {
                using namespace State;
                return {
                    {SwordManAvatarStateType::Disable,            std::make_shared<DisableState>                   (context, callback)},
                    {SwordManAvatarStateType::Idle,               std::make_shared<SwordManAvatarIdleState>         (context, callback)},
                    {SwordManAvatarStateType::Walk,               std::make_shared<SwordManAvatarWalkState>         (context, callback)},
                    {SwordManAvatarStateType::Run,                std::make_shared<SwordManAvatarRunState>          (context, callback)},
                    {SwordManAvatarStateType::Jump,               std::make_shared<SwordManAvatarJumpState>         (context, callback)},
                    {SwordManAvatarStateType::Floating,           std::make_shared<FloatingState>                  (context, callback)},
                    {SwordManAvatarStateType::NormalAttack,       std::make_shared<SwordManAvatarNormalAttackState> (context, callback)},
                    {SwordManAvatarStateType::AttackedShocked,    std::make_shared<AttackedShockedState>            (context, callback)},
                    {SwordManAvatarStateType::DashAttack,         std::make_shared<SwordManAvatarDashAttackState>   (context, callback)},
                    {SwordManAvatarStateType::ClimbToTop,         std::make_shared<SwordManAvatarStateClimbToTop>   (context, callback)},
                    {SwordManAvatarStateType::ArmStretch,         std::make_shared<SwordManAvatarArmStretchState>   (context, callback)},
                    {SwordManAvatarStateType::Chatting,           std::make_shared<SwordManAvatarChattingState>     (context, callback)},
                    {SwordManAvatarStateType::OnEnableReinforce,  std::make_shared<OnEnableReinforceState>          (context, callback)},
                    {SwordManAvatarStateType::OnDisableReinforce, std::make_shared<OnDisableReinforceState>         (context, callback)},
                    {SwordManAvatarStateType::Hurt,               std::make_shared<HurtState>                      (context, callback)},
                    {SwordManAvatarStateType::AvoidRolling,       std::make_shared<AvoidRollingState>               (context, callback)},
                    {SwordManAvatarStateType::Death,              std::make_shared<DeathState>                          (context, callback)},
                    {SwordManAvatarStateType::UseCanon,           std::make_shared<SwordManAvatarUseCannonState>         (context, callback)},
                    {SwordManAvatarStateType::InjuredWalk,        std::make_shared<SwordManAvatarInjuredWalkState>       (context, callback)},
                    {SwordManAvatarStateType::InjuredRun,         std::make_shared<SwordManAvatarInjuredRunState>        (context, callback)},
                };
            },
            SwordManAvatarStateType::Idle,
            SwordManAvatarStateType::Disable,
            isEnable
        );
    }
}

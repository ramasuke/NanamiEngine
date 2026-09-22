#include "SwordManAvatarStateMachine.h"

#include "ArmStretch/SwordManAvatarArmStretchState.h"
#include "Attack/Charge/SwordManAvatarChargeAttackChargingState.h"
#include "Attack/Charge/SwordManAvatarChargeAttackReleaseState.h"
#include "Attack/Dash/SwordManAvatarDashAttackState.h"
#include "Attack/Jump/SwordManAvatarJumpAttackAirState.h"
#include "Attack/Jump/SwordManAvatarJumpAttackLandState.h"
#include "Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "AttackedShocked/SwordManAvatar_AttackedShockedState.h"
#include "AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "Chatting/SwordManAvatarChattingState.h"
#include "ClimbToTop/SwordManAvatarStateClimbToTop.h"
#include "Death/SwordManAvatar_DeathState.h"
#include "DisableState/SwordManAvatar_DisableState.h"
#include "Down/SwordManAvatar_DownState.h"
#include "FallDown/SwordManAvatar_FallDownState.h"
#include "Floating/FloatingState.h"
#include "GetUp/SwordManAvatar_GetUpState.h"
#include "Hurt/SwordManAvatar_HurtState.h"
#include "Idle/SwordManAvatarIdleState.h"
#include "Jump/SwordManAvatarJumpState.h"
#include "InjuredRun/SwordManAvatarInjuredRunState.h"
#include "InjuredWalk/SwordManAvatarInjuredWalkState.h"
#include "Run/SwordManAvatarRunState.h"
#include "UseCanon/SwordManAvatarUseCanonState.h"
#include "WakeUp/SwordManAvatar_WakeUpState.h"
#include "Walk/SwordManAvatarWalkState.h"
#include "WarpIn/SwordManAvatar_WarpInState.h"

#include "../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    namespace
    {
        SwordManAvatarStateType ToSwordManEventSceneState(const EventSceneStateType type)
        {
            switch (type)
            {
            case EventSceneStateType::Idle      : return SwordManAvatarStateType::Idle;
            case EventSceneStateType::Walk      : return SwordManAvatarStateType::Walk;
            case EventSceneStateType::ArmStretch: return SwordManAvatarStateType::ArmStretch;
            case EventSceneStateType::WarpIn    : return SwordManAvatarStateType::WarpIn;
            case EventSceneStateType::GetUp     : return SwordManAvatarStateType::GetUp;
            }
            return SwordManAvatarStateType::Idle;
        }
    }

    SwordManAvatarStateMachine::SwordManAvatarStateMachine(
        StatesFactory factory,
        const SwordManAvatarStateType initialState,
        const SwordManAvatarStateType disableState,
        const bool isEnable)
        : PlayerAvatarStateMachineBase(std::move(factory), initialState, disableState, isEnable)
        , swordManCurrentState_(nullptr)
    {
        baseStateSubscription_.Set(PlayerAvatarStateMachineBase::CurrentState()
            .Subscribe([this](const std::shared_ptr<IPlayerAvatarState>& state)
            {
                swordManCurrentState_.OnNext(
                    std::dynamic_pointer_cast<SwordManAvatarStateBase>(state));
            }));
    }

    void SwordManAvatarStateMachine::OnChangeState(SwordManAvatarStateType type) { Base::OnChangeState(type); }
    void SwordManAvatarStateMachine::OnChangeState(const EventSceneStateType type) { Base::OnChangeState(ToSwordManEventSceneState(type)); }
    void SwordManAvatarStateMachine::OnEnable()  { Base::OnEnable();  }
    void SwordManAvatarStateMachine::OnDisable() { Base::OnDisable(); }

    R4::Observable<std::shared_ptr<SwordManAvatarStateBase>> SwordManAvatarStateMachine::CurrentState() const
    {
        return swordManCurrentState_.AsObservable();
    }

    std::shared_ptr<const SwordManAvatarStateBase> SwordManAvatarStateMachine::CurrentStateValue() const
    {
        return swordManCurrentState_.Value();
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
            playerAvatar->CatchLockOnDetectionArea(),
            playerAvatar->CatchSuccessAvoidRollingParticle(),
            playerAvatar->Resources()
        );

        auto stateMachine = std::make_unique<SwordManAvatarStateMachine>(
            [context](SwordManAvatarStateMachine::OnChangeStateCallback callback)
                -> SwordManAvatarStateMachine::StateMap
            {
                using namespace State;
                const SwordManAvatarStateArgs args(context, callback);
                return {
                    {SwordManAvatarStateType::Disable,            std::make_shared<DisableState>                   (args)},
                    {SwordManAvatarStateType::Idle,               std::make_shared<SwordManAvatarIdleState>         (args)},
                    {SwordManAvatarStateType::Walk,               std::make_shared<SwordManAvatarWalkState>         (args)},
                    {SwordManAvatarStateType::Run,                std::make_shared<SwordManAvatarRunState>          (args)},
                    {SwordManAvatarStateType::Jump,               std::make_shared<SwordManAvatarJumpState>         (args)},
                    {SwordManAvatarStateType::Floating,           std::make_shared<FloatingState>                  (args)},
                    {SwordManAvatarStateType::NormalAttack,       std::make_shared<SwordManAvatarNormalAttackState> (args)},
                    {SwordManAvatarStateType::AttackedShocked,    std::make_shared<AttackedShockedState>            (args)},
                    {SwordManAvatarStateType::DashAttack,         std::make_shared<SwordManAvatarDashAttackState>   (args)},
                    {SwordManAvatarStateType::ClimbToTop,         std::make_shared<SwordManAvatarStateClimbToTop>   (args)},
                    {SwordManAvatarStateType::ArmStretch,         std::make_shared<SwordManAvatarArmStretchState>   (args)},
                    {SwordManAvatarStateType::Chatting,           std::make_shared<SwordManAvatarChattingState>     (args)},
                    {SwordManAvatarStateType::ChargeAttackCharging, std::make_shared<SwordManAvatarChargeAttackChargingState>(args)},
                    {SwordManAvatarStateType::ChargeAttackRelease,  std::make_shared<SwordManAvatarChargeAttackReleaseState> (args)},
                    {SwordManAvatarStateType::Hurt,               std::make_shared<HurtState>                      (args)},
                    {SwordManAvatarStateType::AvoidRolling,       std::make_shared<AvoidRollingState>               (args)},
                    {SwordManAvatarStateType::Death,              std::make_shared<DeathState>                          (args)},
                    {SwordManAvatarStateType::UseCanon,           std::make_shared<SwordManAvatarUseCannonState>         (args)},
                    {SwordManAvatarStateType::InjuredWalk,        std::make_shared<SwordManAvatarInjuredWalkState>       (args)},
                    {SwordManAvatarStateType::InjuredRun,         std::make_shared<SwordManAvatarInjuredRunState>        (args)},
                    {SwordManAvatarStateType::Down,               std::make_shared<DownState>                            (args)},
                    {SwordManAvatarStateType::WakeUp,             std::make_shared<WakeUpState>                          (args)},
                    {SwordManAvatarStateType::FallDown,           std::make_shared<FallDownState>                        (args)},
                    {SwordManAvatarStateType::GetUp,              std::make_shared<GetUpState>                           (args)},
                    {SwordManAvatarStateType::JumpAttackAir,      std::make_shared<SwordManAvatarJumpAttackAirState>     (args)},
                    {SwordManAvatarStateType::JumpAttackLand,     std::make_shared<SwordManAvatarJumpAttackLandState>    (args)},
                    {SwordManAvatarStateType::WarpIn,             std::make_shared<WarpInState>                          (args)},
                };
            },
            SwordManAvatarStateType::Idle,
            SwordManAvatarStateType::Disable,
            isEnable
        );
        status->SetStateMachine(*stateMachine);
        return stateMachine;
    }
}

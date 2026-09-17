#include "MagicCasterAvatarStateMachine.h"

#include "Cast/MagicCasterAvatarCastState.h"
#include "Death/MagicCasterAvatarDeathState.h"
#include "DisableState/MagicCasterAvatarDisableState.h"
#include "Floating/MagicCasterAvatarFloatingState.h"
#include "Hurt/MagicCasterAvatarHurtState.h"
#include "Idle/MagicCasterAvatarIdleState.h"
#include "Jump/MagicCasterAvatarJumpState.h"
#include "Run/MagicCasterAvatarRunState.h"
#include "Walk/MagicCasterAvatarWalkState.h"

#include "../../../../../GamePlay/PlayerAvatar/MagicCaster/MagicCasterAvatar.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    MagicCasterAvatarStateMachine::MagicCasterAvatarStateMachine(
        StatesFactory factory,
        const MagicCasterAvatarStateType initialState,
        const MagicCasterAvatarStateType disableState,
        const bool isEnable)
        : PlayerAvatarStateMachineBase(std::move(factory), initialState, disableState, isEnable)
        , magicCasterCurrentState_(nullptr)
    {
        PlayerAvatarStateMachineBase::CurrentState()
            .subscribe([this](const std::shared_ptr<IPlayerAvatarState>& state)
            {
                magicCasterCurrentState_.get_subscriber().on_next(
                    std::dynamic_pointer_cast<MagicCasterAvatarStateBase>(state));
            });
    }

    void MagicCasterAvatarStateMachine::OnChangeState(MagicCasterAvatarStateType type) { Base::OnChangeState(type); }
    void MagicCasterAvatarStateMachine::OnEnable()  { Base::OnEnable();  }
    void MagicCasterAvatarStateMachine::OnDisable() { Base::OnDisable(); }

    rxcpp::observable<std::shared_ptr<MagicCasterAvatarStateBase>> MagicCasterAvatarStateMachine::CurrentState() const
    {
        return magicCasterCurrentState_.get_observable();
    }

    std::shared_ptr<const MagicCasterAvatarStateBase> MagicCasterAvatarStateMachine::CurrentStateValue() const
    {
        return magicCasterCurrentState_.get_value();
    }

    std::unique_ptr<MagicCasterAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<MagicCasterAvatarStatus     >& status
        , const std::shared_ptr<MagicCasterAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& playerAvatar
        , const std::weak_ptr<PlayerAvatarCameraGroupBase>& cameraGroup
        , const bool isEnable)
    {
        auto context = std::make_shared<MagicCasterAvatarStateContext>(
            status,
            input,
            playerAvatar->Entity(),
            cameraGroup,
            playerAvatar->CastPoint(),
            playerAvatar->Resources()
        );

        auto stateMachine = std::make_unique<MagicCasterAvatarStateMachine>(
            [context](MagicCasterAvatarStateMachine::OnChangeStateCallback callback)
                -> MagicCasterAvatarStateMachine::StateMap
            {
                using namespace State;
                return {
                    {MagicCasterAvatarStateType::Disable,  std::make_shared<DisableState>(context, callback)},
                    {MagicCasterAvatarStateType::Idle,     std::make_shared<IdleState>   (context, callback)},
                    {MagicCasterAvatarStateType::Walk,     std::make_shared<WalkState>   (context, callback)},
                    {MagicCasterAvatarStateType::Run,      std::make_shared<RunState>    (context, callback)},
                    {MagicCasterAvatarStateType::Jump,     std::make_shared<JumpState>   (context, callback)},
                    {MagicCasterAvatarStateType::Floating, std::make_shared<FloatingState>(context, callback)},
                    {MagicCasterAvatarStateType::Cast,     std::make_shared<CastState>   (context, callback)},
                    {MagicCasterAvatarStateType::Hurt,     std::make_shared<HurtState>   (context, callback)},
                    {MagicCasterAvatarStateType::Death,    std::make_shared<DeathState>  (context, callback)},
                };
            },
            MagicCasterAvatarStateType::Idle,
            MagicCasterAvatarStateType::Disable,
            isEnable
        );
        status->SetStateMachine(*stateMachine);
        return stateMachine;
    }
}

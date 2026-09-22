#pragma once
#include "../../StateMachine/PlayerAvatarStateMachineBase.h"
#include "../../StateMachine/EventScene/IPlayerAvatarEventSceneStateMachine.h"
#include "SwordManAvatarStateBase.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStateMachine final : public PlayerAvatarStateMachineBase<SwordManAvatarStateType>,
                                             public IPlayerAvatarEventSceneStateMachine
    {
    public:
        using Base = PlayerAvatarStateMachineBase;
        using Base::StatesFactory;
        using Base::OnChangeStateCallback;
        using Base::StateMap;

        explicit SwordManAvatarStateMachine(
            StatesFactory factory,
            SwordManAvatarStateType initialState,
            SwordManAvatarStateType disableState,
            bool isEnable);

        void OnChangeState(SwordManAvatarStateType type) override;
        void OnChangeState(EventSceneStateType type) override;
        void OnEnable()  override;
        void OnDisable() override;

        NanamiEngine::R4::Observable<std::shared_ptr<SwordManAvatarStateBase>> CurrentState() const;
        [[nodiscard]] std::shared_ptr<const SwordManAvatarStateBase> CurrentStateValue() const;

    private:
        NanamiEngine::R4::ReactiveProperty<std::shared_ptr<SwordManAvatarStateBase>> swordManCurrentState_;
        NanamiEngine::R4::SerialDisposable baseStateSubscription_;
    };

    std::unique_ptr<SwordManAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup
        , bool isEnable);
}

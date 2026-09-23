#pragma once
#include "../../StateMachine/PlayerAvatarStateMachineBase.h"
#include "SwordManAvatarStateBase.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStateMachine final : public PlayerAvatarStateMachineBase<SwordManAvatarStateType>
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

        R4::Observable<std::shared_ptr<SwordManAvatarStateBase>> CurrentState() const;
        [[nodiscard]] std::shared_ptr<const SwordManAvatarStateBase> CurrentStateValue() const;

    private:
        R4::ReactiveProperty<std::shared_ptr<SwordManAvatarStateBase>> swordManCurrentState_;
        R4::SerialDisposable baseStateSubscription_;
    };

    std::unique_ptr<SwordManAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup
        , bool isEnable);
}

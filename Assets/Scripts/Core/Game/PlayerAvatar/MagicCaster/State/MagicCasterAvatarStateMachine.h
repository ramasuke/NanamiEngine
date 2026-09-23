#pragma once
#include "../../StateMachine/PlayerAvatarStateMachineBase.h"
#include "MagicCasterAvatarStateBase.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatar;
}

namespace GameCore::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatarStateMachine final : public PlayerAvatarStateMachineBase<MagicCasterAvatarStateType>
    {
    public:
        using Base = PlayerAvatarStateMachineBase;
        using Base::StatesFactory;
        using Base::OnChangeStateCallback;
        using Base::StateMap;

        explicit MagicCasterAvatarStateMachine(
            StatesFactory factory,
            MagicCasterAvatarStateType initialState,
            MagicCasterAvatarStateType disableState,
            bool isEnable);

        void OnChangeState(MagicCasterAvatarStateType type) override;
        void OnChangeState(EventSceneStateType type) override;

        NanamiEngine::R4::Observable<std::shared_ptr<MagicCasterAvatarStateBase>> CurrentState() const;
        [[nodiscard]] std::shared_ptr<const MagicCasterAvatarStateBase> CurrentStateValue() const;

    private:
        NanamiEngine::R4::ReactiveProperty<std::shared_ptr<MagicCasterAvatarStateBase>> magicCasterCurrentState_;
        NanamiEngine::R4::SerialDisposable baseStateSubscription_;
    };

    std::unique_ptr<MagicCasterAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<MagicCasterAvatarStatus     >& status
        , const std::shared_ptr<MagicCasterAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& playerAvatar
        , const std::weak_ptr<PlayerAvatarCameraGroupBase>& cameraGroup
        , bool isEnable);
}

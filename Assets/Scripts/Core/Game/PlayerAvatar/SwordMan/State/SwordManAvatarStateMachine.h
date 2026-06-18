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
        void OnEnable()  override;
        void OnDisable() override;

        rxcpp::observable<std::shared_ptr<SwordManAvatarStateBase>> CurrentState() const;

    private:
        rxcpp::subjects::behavior<std::shared_ptr<SwordManAvatarStateBase>> swordManCurrentState_;
    };

    std::unique_ptr<SwordManAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup
        , bool isEnable);
}

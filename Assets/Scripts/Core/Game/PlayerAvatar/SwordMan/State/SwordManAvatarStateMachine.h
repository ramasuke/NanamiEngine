#pragma once
#include "../../StateMachine/PlayerAvatarStateMachine.h"
#include "SwordManAvatarStateBase.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStateMachine final : public PlayerAvatarStateMachine
    {
    public:
        explicit SwordManAvatarStateMachine(
            StatesFactory factory,
            SwordManAvatarStateType initialState,
            SwordManAvatarStateType disableState);

        rxcpp::observable<std::shared_ptr<SwordManAvatarStateBase>> CurrentState();

    private:
        rxcpp::subjects::behavior<std::shared_ptr<SwordManAvatarStateBase>> swordManCurrentState_;
    };

    std::unique_ptr<SwordManAvatarStateMachine> CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup);
}

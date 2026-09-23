#include "PlayerAvatar_OtherPlayer_StatusPresenter.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::PlayerAvatar::OtherPlayer
{
    void StatusPresenter::Initialize(
        const Ui::PlayerStatus& playerStatusView,
        const GameCore::PlayerAvatar::IPlayerAvatarStatus& playerStatusModel)
    {
        StatusPresenterBase::Initialize(
            playerStatusView,
            playerStatusModel);
    }

    void StatusPresenter::OnDrawGui()
    {
        
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::OtherPlayer::StatusPresenter);
#pragma endregion

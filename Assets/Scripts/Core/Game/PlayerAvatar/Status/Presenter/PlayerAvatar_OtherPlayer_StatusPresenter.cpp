#include "PlayerAvatar_OtherPlayer_StatusPresenter.h"

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

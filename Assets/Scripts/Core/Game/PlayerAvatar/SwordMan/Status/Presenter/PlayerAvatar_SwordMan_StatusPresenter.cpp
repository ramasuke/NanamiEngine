#include "PlayerAvatar_SwordMan_StatusPresenter.h"

#include "../SwordManAvatarStatus.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    void StatusPresenter::Initialize(
        const Ui::PlayerStatus& playerStatusView,
        const GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus& playerStatusModel)
    {
        StatusPresenterBase::Initialize(
            playerStatusView,
            playerStatusModel);
    }

    void StatusPresenter::OnDrawGui()
    {
        
    }
}

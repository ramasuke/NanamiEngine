#include "MagicCasterAvatarChattingState.h"

#include "../../../../../../GamePlay/PlayerAvatar/InteractableArea/InteractableArea.h"
#include "../../../Interactable/IPlayerInteractable.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    void ChattingState::DoEnter()
    {
        InteractableArea().CatchInteractTarget().lock()->OnInteract();
        OnChangeState(MagicCasterAvatarStateType::Idle);
    }

    void ChattingState::DoFixedUpdate()
    {

    }

    void ChattingState::DoUpdate()
    {

    }

    void ChattingState::DoExit()
    {

    }
}

#include "SwordManAvatarChattingState.h"

#include "../../../../../../GamePlay/PlayerAvatar/InteractableArea/InteractableArea.h"
#include "../../../Interactable/IPlayerInteractable.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarChattingState::DoEnter()
    {
        InteractableArea().CatchInteractTarget().lock()->OnInteract();
        OnChangeState(SwordManAvatarStateType::Idle);
    }

    void SwordManAvatarChattingState::DoFixedUpdate()
    {

    }

    void SwordManAvatarChattingState::DoUpdate()
    {

    }

    void SwordManAvatarChattingState::DoExit()
    {

    }
}

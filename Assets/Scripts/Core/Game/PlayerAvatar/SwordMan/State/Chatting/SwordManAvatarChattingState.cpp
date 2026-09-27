#include "SwordManAvatarChattingState.h"

#include "../../../../../../GamePlay/PlayerAvatar/InteractableArea/InteractableArea.h"
#include "../../../Interactable/IPlayerInteractable.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarChattingState::DoEnter()
    {
        const auto target = InteractableArea().CatchInteractTarget().lock();
        OnChangeState(SwordManAvatarStateType::Idle);
        if (target)
        {
            target->OnInteract();
        }
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

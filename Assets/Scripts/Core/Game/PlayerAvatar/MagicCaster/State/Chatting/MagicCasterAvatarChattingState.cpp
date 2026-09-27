#include "MagicCasterAvatarChattingState.h"

#include "../../../../../../GamePlay/PlayerAvatar/InteractableArea/InteractableArea.h"
#include "../../../Interactable/IPlayerInteractable.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    void ChattingState::DoEnter()
    {
        const auto target = InteractableArea().CatchInteractTarget().lock();
        OnChangeState(MagicCasterAvatarStateType::Idle);
        if (target)
        {
            target->OnInteract();
        }
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

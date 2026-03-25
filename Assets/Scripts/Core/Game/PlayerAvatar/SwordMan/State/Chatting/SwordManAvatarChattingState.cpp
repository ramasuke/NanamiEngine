#include "SwordManAvatarChattingState.h"

#include "../../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../Chattable/IPlayerChattable.h"
#include "../Idle/SwordManAvatarIdleState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarChattingState::DoEnter()
    {
        ChattableArea().CatchChatTarget().lock()->OnChat();
        OnChangeState<SwordManAvatarIdleState>();
    }

    void SwordManAvatarChattingState::DoUpdate()
    {
        
    }

    void SwordManAvatarChattingState::DoExit()
    {
        
    }
}

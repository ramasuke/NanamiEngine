#include "SwordManAvatarChattingState.h"

#include "../../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../Chattable/IPlayerChattable.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarChattingState::DoEnter()
    {
        ChattableArea().CatchChatTarget().lock()->OnChat();
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

#include "MagicCasterAvatarChattingState.h"

#include "../../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../../Chattable/IPlayerChattable.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    void ChattingState::DoEnter()
    {
        ChattableArea().CatchChatTarget().lock()->OnChat();
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

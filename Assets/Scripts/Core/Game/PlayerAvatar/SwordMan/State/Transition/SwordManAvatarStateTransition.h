#pragma once
#include "SwordManAvatarInput.h"
#include "SwordManAvatarStateAction.h"
#include "../SwordManAvatarStateType.h"
#include "../../../State/Transition/PlayerAvatarStateTransition.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    using ISwordManAvatarTransitionVisitor =
        IPlayerAvatarTransitionVisitor<SwordManAvatarStateType, SwordManAvatarInput, SwordManAvatarStateAction>;
}

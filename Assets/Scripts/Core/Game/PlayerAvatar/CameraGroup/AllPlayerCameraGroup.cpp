#include "AllPlayerCameraGroup.h"

namespace GameCore::PlayerAvatar
{
    AllPlayerCameraGroup::AllPlayerCameraGroup(
        std::weak_ptr<SwordMan::SwordManAvatarCameraGroup> swordman)
        : swordman_(std::move(swordman))
    {
    }
}

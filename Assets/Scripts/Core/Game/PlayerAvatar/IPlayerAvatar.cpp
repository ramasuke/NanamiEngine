#include "IPlayerAvatar.h"

const std::vector<std::weak_ptr<GameCore::IPlayerAvatar>>& GameCore::IPlayerAvatar::PlayerAvatars()
{
    return PlayerAvatars_();
}

std::vector<std::weak_ptr<GameCore::IPlayerAvatar>>& GameCore::IPlayerAvatar::PlayerAvatars_()
{
    static std::vector<std::weak_ptr<IPlayerAvatar>> playerAvatars;
    return playerAvatars;
}

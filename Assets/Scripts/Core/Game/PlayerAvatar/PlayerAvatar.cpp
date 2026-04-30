#include "PlayerAvatar.h"

std::shared_ptr<GameCore::IPlayerAvatar> GameCore::PlayerAvatar::Owner()
{
    //TODO: Network上の自身が操作しているPlayerを取得するように変更必須
    return IPlayerAvatar::PlayerAvatars().at(0).lock();
}

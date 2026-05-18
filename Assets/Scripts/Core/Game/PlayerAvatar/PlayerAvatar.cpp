#include "PlayerAvatar.h"

#include "../../../../../Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Type/PlayerAvatarType.h"

namespace GameCore::PlayerAvatar
{
    std::shared_ptr<IPlayerAvatar> Owner()
    {
        //TODO: Network上の自身が操作しているPlayerを取得するように変更必須
        return IPlayerAvatar::PlayerAvatars().at(0).lock();
    }

    void SaveType(const IPlayerAvatar& playerAvatar)
    {
        LocalPrefs::SaveWithPath(PLAYER_AVATAR_TYPE_FILE_PATH, PLAYER_AVATAR_TYPE_FILE_KEY, playerAvatar.Type());
    }

    PlayerAvatarType LoadType()
    {
        return LocalPrefs::LoadWithPath<PlayerAvatarType>(PLAYER_AVATAR_TYPE_FILE_PATH, PLAYER_AVATAR_TYPE_FILE_KEY);
    }
}

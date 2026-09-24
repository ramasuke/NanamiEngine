#include "PlayerAvatar.h"

#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Type/PlayerAvatarType.h"

namespace GameCore::PlayerAvatar
{
    std::shared_ptr<IPlayerAvatar> Owner()
    {
        for (const auto& weakAvatar : IPlayerAvatar::PlayerAvatars())
        {
            if (auto avatar = weakAvatar.lock(); avatar && avatar->IsOwner())
                return avatar;
        }
        return nullptr;
    }

    void SaveType(const IPlayerAvatar& playerAvatar)
    {
        LocalPrefs::SaveWithPath(PLAYER_AVATAR_TYPE_FILE_PATH, PLAYER_AVATAR_TYPE_FILE_KEY, playerAvatar.Type());
    }

    void SaveType(const PlayerAvatarType type)
    {
        LocalPrefs::SaveWithPath(PLAYER_AVATAR_TYPE_FILE_PATH, PLAYER_AVATAR_TYPE_FILE_KEY, type);
    }

    PlayerAvatarType LoadType()
    {
        return LocalPrefs::LoadWithPath<PlayerAvatarType>(PLAYER_AVATAR_TYPE_FILE_PATH, PLAYER_AVATAR_TYPE_FILE_KEY);
    }
}

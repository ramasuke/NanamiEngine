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

    void SelectedPlayerAvatarType::Save(const IPlayerAvatar& playerAvatar)
    {
        Save(playerAvatar.Type());
    }

    void SelectedPlayerAvatarType::Save(const PlayerAvatarType type)
    {
        LocalPrefs::SaveWithPath(FILE_PATH, FILE_KEY, type);
    }

    PlayerAvatarType SelectedPlayerAvatarType::Load()
    {
        return LocalPrefs::LoadWithPath<PlayerAvatarType>(FILE_PATH, FILE_KEY);
    }
}

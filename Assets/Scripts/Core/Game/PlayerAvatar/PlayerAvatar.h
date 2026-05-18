#pragma once
#include <filesystem>
#include <memory>
#include <type_traits>

#include "IPlayerAvatar.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::PlayerAvatar
{
    constexpr auto PLAYER_AVATAR_TYPE_FILE_PATH = "PlayerAvatar/Type";
    constexpr auto PLAYER_AVATAR_TYPE_FILE_KEY  = "Info";
    
    template <class T>
    concept PlayerAvatarT = std::is_base_of_v<IPlayerAvatar, std::remove_cv_t<std::remove_reference_t<T>>>;

    [[nodiscard]] std::shared_ptr<IPlayerAvatar> Owner();

    template<PlayerAvatarT T>
    std::shared_ptr<IPlayerAvatar> Owner()
    {
        //TODO: Network上の自身が操作しているPlayerを取得するように変更必須
        return IPlayerAvatar::PlayerAvatars().at(0).lock();
    }
    
    template<PlayerAvatarT PlayerAvatarT>
    std::shared_ptr<PlayerAvatarT> TryWhetherPlayerT(const std::shared_ptr<IPlayerAvatar>& playerAvatar)
    {
        return std::dynamic_pointer_cast<PlayerAvatarT>(playerAvatar);
    }

    void SaveType(const IPlayerAvatar& playerAvatar);
    PlayerAvatarType LoadType();
}

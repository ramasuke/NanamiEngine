#pragma once
#include <memory>
#include <type_traits>

#include "IPlayerAvatar.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::PlayerAvatar
{
    template <class T>
    concept PlayerAvatarT = std::is_base_of_v<IPlayerAvatar, std::remove_cv_t<std::remove_reference_t<T>>>;

    [[nodiscard]] std::shared_ptr<IPlayerAvatar> Owner();

    template<PlayerAvatarT T>
    std::shared_ptr<IPlayerAvatar> Owner()
    {
        //TODO: Network上の自身が操作しているPlayerを取得するように変更必須
        return IPlayerAvatar::PlayerAvatars().at(0).lock();
    }
    
    template<typename PlayerAvatarT>
    requires std::is_base_of_v<IPlayerAvatar, std::remove_cv_t<std::remove_reference_t<PlayerAvatarT>>>
    std::shared_ptr<PlayerAvatarT> TryWhetherPlayerT(const std::shared_ptr<IPlayerAvatar>& playerAvatar)
    {
        return std::dynamic_pointer_cast<PlayerAvatarT>(playerAvatar);
    }
}

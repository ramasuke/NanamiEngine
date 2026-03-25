#pragma once
#include <memory>
#include <type_traits>

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::PlayerAvatar
{
    template<typename PlayerAvatarT>
    requires std::is_base_of_v<IPlayerAvatar, std::remove_cv_t<std::remove_reference_t<PlayerAvatarT>>>
    std::shared_ptr<PlayerAvatarT> TryWhetherPlayerT(const std::shared_ptr<IPlayerAvatar>& playerAvatar)
    {
        return std::dynamic_pointer_cast<PlayerAvatarT>(playerAvatar);
    }
}

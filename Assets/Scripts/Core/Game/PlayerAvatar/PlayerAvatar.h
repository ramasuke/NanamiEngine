#pragma once
#include <filesystem>
#include <memory>
#include <type_traits>

#include "IPlayerAvatar.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::PlayerAvatar
{
    template <class T>
    concept PlayerAvatarT = std::is_base_of_v<IPlayerAvatar, std::remove_cv_t<std::remove_reference_t<T>>>;

    /** @brief この PC で操作しているアバター。いなければ nullptr */
    [[nodiscard]] std::shared_ptr<IPlayerAvatar> Owner();

    template<PlayerAvatarT PlayerAvatarT>
    std::shared_ptr<PlayerAvatarT> TryWhetherPlayerT(const std::shared_ptr<IPlayerAvatar>& playerAvatar)
    {
        return std::dynamic_pointer_cast<PlayerAvatarT>(playerAvatar);
    }

    /** @brief 選んでいるアバターの種類。LocalPrefs に保存・読込する */
    class SelectedPlayerAvatarType final
    {
    public:
        SelectedPlayerAvatarType() = delete;

        static void Save(const IPlayerAvatar& playerAvatar);
        static void Save(PlayerAvatarType type);
        // NOTE: 既定値を持たないので、保存前に読むと失敗する
        [[nodiscard]] static PlayerAvatarType Load();

    private:
        static constexpr auto FILE_PATH = "PlayerAvatar/Type";
        static constexpr auto FILE_KEY  = "Info";
    };
}

#pragma once
#include <type_traits>

#include "IPlayerAvatarStatus.h"
#include "../../../../../../Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar
{
    constexpr auto PLAYER_AVATAR_STATUS_FILE_KEY  = "Info";
    
    template<typename StatusT, typename TraitsT>
    void SaveStatus(const std::shared_ptr<StatusT>& status)
    {
        LocalPrefs::SaveWithPath(TraitsT::STATUS_SAVE_FILE_PATH, PLAYER_AVATAR_STATUS_FILE_KEY, status);
    }
    
    template<typename StatusT, typename TraitsT>
    std::shared_ptr<StatusT> LoadStatus()
    {
        return LocalPrefs::LoadWithPath<std::shared_ptr<StatusT>>(TraitsT::STATUS_SAVE_FILE_PATH, PLAYER_AVATAR_STATUS_FILE_KEY);
    }
}

#pragma once
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "../RequireType/RequireType.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar
{
    constexpr auto PLAYER_AVATAR_STATUS_FILE_KEY  = "Info";
    
    template<typename StatusT, typename TraitsT>
    void SaveStatus(const std::shared_ptr<StatusT>& status)
    {
        LocalPrefs::SaveWithPath(TraitsT::STATUS_SAVE_FILE_PATH, PLAYER_AVATAR_STATUS_FILE_KEY, status);
    }
    
    // NOTE: ファイルが無ければ例外を投げず既定コンストラクタの初期値を返す
    template<typename TraitsT>
    std::shared_ptr<RequireType::Status<TraitsT>> LoadStatus()
    {
        return LocalPrefs::LoadOrDefaultWithPath<std::shared_ptr<RequireType::Status<TraitsT>>>(
            TraitsT::STATUS_SAVE_FILE_PATH,
            PLAYER_AVATAR_STATUS_FILE_KEY,
            std::make_shared<RequireType::Status<TraitsT>>());  
    }
}

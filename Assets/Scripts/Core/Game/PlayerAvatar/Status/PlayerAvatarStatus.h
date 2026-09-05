#pragma once
#include "../../../../../../Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "../RequireType/RequireType.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar
{
    constexpr auto PLAYER_AVATAR_STATUS_FILE_KEY  = "Info";
    
    template<typename StatusT, typename TraitsT>
    void SaveStatus(const std::shared_ptr<StatusT>& status)
    {
        LocalPrefs::SaveWithPath(TraitsT::STATUS_SAVE_FILE_PATH, PLAYER_AVATAR_STATUS_FILE_KEY, status);
    }
    
    // NOTE: 未セーブ(初回プレイ等)でファイルが無い場合は例外を投げず、RequireType::Status<TraitsT>の
    // 引数無しコンストラクタ(ハードコードされた初期値)にフォールバックする
    template<typename TraitsT>
    std::shared_ptr<RequireType::Status<TraitsT>> LoadStatus()
    {
        return LocalPrefs::LoadOrDefaultWithPath<std::shared_ptr<RequireType::Status<TraitsT>>>(
            TraitsT::STATUS_SAVE_FILE_PATH,
            PLAYER_AVATAR_STATUS_FILE_KEY,
            std::make_shared<RequireType::Status<TraitsT>>());
    }
}

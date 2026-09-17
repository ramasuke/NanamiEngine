#include "MagicCasterAvatarTraits.h"

#include "../../../../../../Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "../../Status/PlayerAvatarStatus.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    std::unique_ptr<MagicCasterAvatarTraits::StateMachine>
    MagicCasterAvatarTraits::CreateStateMachine(
        const std::shared_ptr<MagicCasterAvatarStatus     >& status,
        const std::shared_ptr<MagicCasterAvatarInputAction>& input,
        const std::shared_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& playerAvatar,
        const std::weak_ptr<PlayerAvatarCameraGroupBase>& cameraGroup,
        const bool isEnable)
    {
        return std::move(MagicCaster::CreateStateMachine(
            status,
            input,
            playerAvatar,
            cameraGroup,
            isEnable));
    }

    REGISTER_LOCAL_PREF_WITH_PATH(
        GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus,
        PLAYER_AVATAR_STATUS_FILE_KEY,
        MagicCasterAvatarStatus(),
        MagicCasterAvatarTraits::STATUS_SAVE_FILE_PATH)
}

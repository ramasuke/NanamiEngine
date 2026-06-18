#include "SwordManAvatarTraits.h"

#include "../../../../../../../Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "../../Status/PlayerAvatarStatus.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    std::unique_ptr<SwordManAvatarTraits::StateMachine>
    SwordManAvatarTraits::CreateStateMachine(
        const std::shared_ptr<SwordManAvatarStatus     >& status, 
        const std::shared_ptr<SwordManAvatarInputAction>& input, 
        const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar, 
        const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup,
        const bool isEnable)
    {
        return std::move(SwordMan::CreateStateMachine(
            status,
            input,
            playerAvatar,
            cameraGroup,
            isEnable));
    }
    
    REGISTER_LOCAL_PREF_WITH_PATH(
        GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus,
        PLAYER_AVATAR_STATUS_FILE_KEY,
        SwordManAvatarStatus(),
        SwordManAvatarTraits::STATUS_SAVE_FILE_PATH)
}

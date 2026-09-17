#pragma once
#include "../../Animator/PlayerAvatarAnimator.h"
#include "../../CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "../State/MagicCasterAvatarStateBase.h"
#include "../Status/MagicCasterAvatarStatus.h"
#include "../Animation/MagicCasterAvatarAnimation.h"
#include "../InputAction/MagicCasterAvatarInputAction.h"
#include "../State/MagicCasterAvatarStateMachine.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    struct MagicCasterAvatarTraits final
    {
        using Animator     = PlayerAvatarAnimator<AnimationType>;
        using Status       = MagicCasterAvatarStatus;
        using StateMachine = MagicCasterAvatarStateMachine;
        using State        = MagicCasterAvatarStateBase;
        using InputAction  = MagicCasterAvatarInputAction;
        using CameraGroup  = PlayerAvatarCameraGroupBase;

        static constexpr auto STATUS_SAVE_FILE_PATH = "PlayerAvatar/MagicCasterStatus";

        static std::unique_ptr<StateMachine> CreateStateMachine(
          const std::shared_ptr<MagicCasterAvatarStatus     >& status
        , const std::shared_ptr<MagicCasterAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& playerAvatar
        , const std::weak_ptr<PlayerAvatarCameraGroupBase>& cameraGroup
        , bool isEnable);
    };
}

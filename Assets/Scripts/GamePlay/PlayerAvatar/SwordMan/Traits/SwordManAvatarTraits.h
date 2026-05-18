#pragma once
#include "../../../../Core/Game/PlayerAvatar/Animator/PlayerAvatarAnimator.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/State/SwordManAvatarStateBase.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/Status/SwordManAvatarStatus.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/Animation/SwordManAvatarAnimation.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/CameraGroup/SwordManAvatarCameraGroup.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/InputAction/SwordManAvatarInputAction.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/State/SwordManAvatarStateMachine.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    struct SwordManAvatarTraits final
    {
        using Animator     = PlayerAvatarAnimator<AnimationType>;
        using Status       = SwordManAvatarStatus;
        using StateMachine = SwordManAvatarStateMachine;
        using State        = SwordManAvatarStateBase;
        using InputAction  = SwordManAvatarInputAction;
        using CameraGroup  = SwordManAvatarCameraGroup;
        
        static constexpr auto STATUS_SAVE_FILE_PATH = "PlayerAvatar/SwordManStatus";
        static std::unique_ptr<StateMachine> CreateStateMachine(
          const std::shared_ptr<SwordManAvatarStatus     >& status
        , const std::shared_ptr<SwordManAvatarInputAction>& input
        , const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
        , const std::weak_ptr<SwordManAvatarCameraGroup>& cameraGroup);
    };
}

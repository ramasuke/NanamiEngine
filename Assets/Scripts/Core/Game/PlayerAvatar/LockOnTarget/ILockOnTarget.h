#pragma once
#include "Packages/Cinemachine/VirtualCamera/Behaviour/ILockOnCameraTarget.h"

namespace GameCore::PlayerAvatar
{
    // NOTE: ロックオン対象の印。位置は ILockOnCameraTarget::LockOnPosition() で渡す
    class ILockOnTarget : public NanamiEngine::CineMachine::ILockOnCameraTarget
    {
    public:
        ~ILockOnTarget() override = default;
    };
}

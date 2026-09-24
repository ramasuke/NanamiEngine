#pragma once
#include "Packages/Cinemachine/VirtualCamera/Behaviour/IVirtualCameraTarget.h"

namespace GameCore::PlayerAvatar
{
    // このGameObjectがロックオン対象になり得ることを示すインターフェース
    // NOTE: ロックオン位置は ILockOnCameraTarget::LockOnPosition() で渡す（ロックオンカメラもこれを使う）。
    //       位置の取得は ILockOnTarget::PositionOf(target)。持たないオブジェクトは自身の位置を返す
    class ILockOnTarget : public NanamiEngine::CineMachine::ILockOnCameraTarget
    {
    public:
        ~ILockOnTarget() override = default;
    };
}

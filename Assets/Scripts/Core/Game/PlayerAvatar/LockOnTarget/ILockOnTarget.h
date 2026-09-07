#pragma once

namespace GameCore::PlayerAvatar
{
    // このGameObjectがロックオン対象になり得ることを示すマーカーインターフェース
    class ILockOnTarget
    {
    public:
        virtual ~ILockOnTarget() = default;
    };
}

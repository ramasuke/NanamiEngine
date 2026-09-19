#pragma once
#include "../../../../../../Libs/glm/vec3.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::PlayerAvatar
{
    // このGameObjectがロックオン対象になり得ることを示すインターフェース
    class ILockOnTarget
    {
    public:
        virtual ~ILockOnTarget() = default;
        // 照準を重ねたり視線を通したりする位置（ワールド座標）
        [[nodiscard]] virtual glm::vec3 LockOnPosition() = 0;
    };

    // ILockOnTarget の子孫に付け、その本体をロック中に切り替えで狙える部位
    class ILockOnPart
    {
    public:
        virtual ~ILockOnPart() = default;
        [[nodiscard]] virtual glm::vec3 LockOnPosition() = 0;
    };

    // ILockOnTarget / ILockOnPart を持たないオブジェクトは自身の位置を返す
    [[nodiscard]] glm::vec3 LockOnPositionOf(NanamiEngine::Module::GameObject::IGameObject& target);
}

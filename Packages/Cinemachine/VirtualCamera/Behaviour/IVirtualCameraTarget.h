#pragma once
#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace NanamiEngine::CineMachine
{
    /**
     * @brief 追従・注視される側が、カメラに追わせたい位置を渡すためのインターフェース
     * @details 対象のGameObjectにこれを実装したコンポーネントがあれば、カメラはTransformの代わりにこの位置を使う。
     *          描画だけ補間しているオブジェクトなど、Transformと見た目の位置がずれるものが実装する
     */
    class IVirtualCameraTarget
    {
    public:
        virtual ~IVirtualCameraTarget() = default;
        [[nodiscard]] virtual glm::vec3 CameraTargetPosition() const = 0;

        /** @brief targetがIVirtualCameraTargetを持っていればその位置、無ければTransformのワールド座標 */
        [[nodiscard]] static glm::vec3 PositionOf(Module::GameObject::IGameObject& target)
        {
            if (const auto cameraTarget = target.Components().Catch<IVirtualCameraTarget>().lock())
                return cameraTarget->CameraTargetPosition();
            return target.Transform().GetWorldPos();
        }
    };

    /**
     * @brief ロックオンされる側が、照準を重ねたい位置を渡すためのインターフェース
     * @details LockOnCameraBehaviour はこの位置も画角に収める。ゲーム側のロックオン対象が実装する
     */
    class ILockOnCameraTarget
    {
    public:
        virtual ~ILockOnCameraTarget() = default;
        // 照準を重ねたり視線を通したりする位置（ワールド座標）
        [[nodiscard]] virtual glm::vec3 LockOnPosition() = 0;

        /** @brief targetがILockOnCameraTargetを持っていればその位置、無ければTransformのワールド座標 */
        [[nodiscard]] static glm::vec3 PositionOf(Module::GameObject::IGameObject& target)
        {
            if (const auto lockOnTarget = target.Components().Catch<ILockOnCameraTarget>().lock())
                return lockOnTarget->LockOnPosition();
            return target.Transform().GetWorldPos();
        }
    };
}

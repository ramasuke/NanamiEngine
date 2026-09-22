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
    };

    /** @brief targetがIVirtualCameraTargetを持っていればその位置、無ければTransformのワールド座標 */
    [[nodiscard]] inline glm::vec3 CameraTargetPositionOf(Module::GameObject::IGameObject& target)
    {
        if (const auto cameraTarget = target.Components().Catch<IVirtualCameraTarget>().lock())
            return cameraTarget->CameraTargetPosition();
        return target.Transform().GetWorldPos();
    }
}

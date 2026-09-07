#include "LockOnDetectionArea.h"

#include "../../../Core/Game/PlayerAvatar/LockOnTarget/ILockOnTarget.h"

namespace GamePlay::PlayerAvatar
{
    void LockOnDetectionArea::OnTriggerEnter(
        const Physics::Manifold& contactManifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        if (gameObject->Components().Catch<ILockOnTarget>().expired())
            return;

        candidates_.push_back(gameObject);
    }

    void LockOnDetectionArea::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        //TODO: ここ消せる、gameObjectがnullなのはonTriggerExitを呼び出す管理部分のengine側のバグ
        if (!gameObject)
            return;

        std::erase_if(candidates_, [&](const std::weak_ptr<GameObject::IGameObject>& w)
        {
            return w.lock() == gameObject;
        });
    }

    const std::vector<std::weak_ptr<GameObject::IGameObject>>& LockOnDetectionArea::Candidates() const
    {
        auto& candidates = const_cast<std::vector<std::weak_ptr<GameObject::IGameObject>>&>(candidates_);
        std::erase_if(candidates, [](const std::weak_ptr<GameObject::IGameObject>& w) { return w.expired(); });
        return candidates_;
    }

    void LockOnDetectionArea::OnDrawGui()
    {
        ImGui::TextUnformatted("Lock On Detection Area");
        ImGui::Text("Candidates: %d", static_cast<int>(candidates_.size()));
        ImGui::Separator();
    }
}

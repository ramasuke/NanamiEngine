#include "ChattableArea.h"

#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Core/Game/PlayerAvatar/Chattable/IPlayerChattable.h"

namespace GamePlay::PlayerAvatar
{
    std::weak_ptr<IPlayerChattable> 
    ChattableArea::CatchChatTarget()
    {
        std::shared_ptr<IPlayerChattable> nearest = nullptr;
        float nearestDistSq = std::numeric_limits<float>::max();
    
        for (auto it = playerChattableTargets_.begin(); it != playerChattableTargets_.end();)
        {
            if (const auto target = it->lock())
            {
                const glm::vec3 direction = target->ChattableTransform().GetWorldPos() - Transform().GetWorldPos();
                if (const float distSq = glm::dot(direction, direction); distSq < nearestDistSq)
                {
                    nearestDistSq = distSq;
                    nearest = target;
                }
    
                ++it;
            }
            else
            {
                it = playerChattableTargets_.erase(it);
            }
        }
    
        return std::weak_ptr(nearest);
    }
    
    void ChattableArea::OnTriggerEnter(
        const Physics::Manifold& contactManifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto playerChattable = gameObject->Components().Catch<IPlayerChattable>();
        if (playerChattable.expired())
            return;

        playerChattableTargets_.push_back(playerChattable);
        playerChattable.lock()->OnChattable();
    }
    
    void ChattableArea::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        //TODO: ここ消せる、gameObjectがnullなのはonTriggerExitを呼び出す管理部分のengine側のバグ
        if (!gameObject)
            return;
        
        const auto leaving = gameObject->Components().Catch<IPlayerChattable>().lock();
        if (!leaving)
            return;
    
        std::erase_if(playerChattableTargets_, [&](const std::weak_ptr<IPlayerChattable>& w){
            return w.lock() == leaving;
        });
        leaving->OnExitChattable();
    }
    
    
    void ChattableArea::OnDrawGui()
    {
        ImGui::TextUnformatted("Chattable Area");

        int aliveCount   = 0;
        int expiredCount = 0;

        for (const auto& w : playerChattableTargets_)
        {
            if (w.expired())
                ++expiredCount;
            else
                ++aliveCount;
        }

        ImGui::Text("Targets: %d (Alive: %d / Expired: %d)",
                    static_cast<int>(playerChattableTargets_.size()),
                    aliveCount,
                    expiredCount);

        ImGui::Separator();

        //配列の中身表示
        if (ImGui::TreeNode("Stored Targets"))
        {
            int index = 0;
            const glm::vec3 selfPos = Transform().GetWorldPos();

            for (const auto& w : playerChattableTargets_)
            {
                ImGui::PushID(index);

                if (const auto target = w.lock())
                {
                    const glm::vec3 dir = target->ChattableTransform().GetWorldPos() - selfPos;
                    const float dist = glm::length(dir);

                    ImGui::Text(" [%d] Alive", index);
                    ImGui::SameLine();
                    ImGui::Text("Distance: %.2f", dist);
                }
                else
                {
                    ImGui::TextDisabled(" [%d] Expired", index);
                }

                ImGui::PopID();
                ++index;
            }

            ImGui::TreePop();
        }

        ImGui::Separator();

        //最近傍
        if (const auto nearest = CatchChatTarget().lock())
        {
            const glm::vec3 direction = nearest->ChattableTransform().GetWorldPos() - Transform().GetWorldPos();
            const float dist = glm::length(direction);

            ImGui::Text("Nearest Target");
            ImGui::Text("Distance: %.2f", dist);
        }
        else
        {
            ImGui::TextDisabled("No valid target");
        }
    }
}

#include "InteractableArea.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Core/Game/PlayerAvatar/Interactable/IPlayerInteractable.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::PlayerAvatar
{
    std::weak_ptr<IPlayerInteractable> 
    InteractableArea::CatchInteractTarget()
    {
        std::shared_ptr<IPlayerInteractable> nearest = nullptr;
        float nearestDistSq = std::numeric_limits<float>::max();
    
        for (auto it = playerInteractableTargets_.begin(); it != playerInteractableTargets_.end();)
        {
            if (const auto target = it->lock())
            {
                if (!target->CanInteract())
                {
                    ++it;
                    continue;
                }

                const glm::vec3 direction = target->InteractableTransform().GetWorldPos() - Transform().GetWorldPos();
                if (const float distSq = glm::dot(direction, direction); distSq < nearestDistSq)
                {
                    nearestDistSq = distSq;
                    nearest = target;
                }
    
                ++it;
            }
            else
            {
                it = playerInteractableTargets_.erase(it);
            }
        }
    
        return std::weak_ptr(nearest);
    }
    
    void InteractableArea::OnTriggerEnter(
        const Physics::Manifold& contactManifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto playerInteractable = gameObject->Components().Catch<IPlayerInteractable>();
        if (playerInteractable.expired())
            return;

        playerInteractableTargets_.push_back(playerInteractable);
        const auto owner = GameCore::PlayerAvatar::Owner();
        isNotifyingOwner_ = owner && &owner->InteractableArea() == this;
        playerInteractable.lock()->OnInteractable();
        isNotifyingOwner_ = false;
    }
    
    void InteractableArea::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        //TODO: ここ消せる、gameObjectがnullなのはonTriggerExitを呼び出す管理部分のengine側のバグ
        if (!gameObject)
            return;
        
        const auto leaving = gameObject->Components().Catch<IPlayerInteractable>().lock();
        if (!leaving)
            return;
    
        std::erase_if(playerInteractableTargets_, [&](const std::weak_ptr<IPlayerInteractable>& w){
            return w.lock() == leaving;
        });
        leaving->OnExitInteractable();
    }
    
    
    void InteractableArea::OnDrawGui()
    {
        ImGui::TextUnformatted("Interactable Area");

        int aliveCount   = 0;
        int expiredCount = 0;

        for (const auto& w : playerInteractableTargets_)
        {
            if (w.expired())
                ++expiredCount;
            else
                ++aliveCount;
        }

        ImGui::Text("Targets: %d (Alive: %d / Expired: %d)",
                    static_cast<int>(playerInteractableTargets_.size()),
                    aliveCount,
                    expiredCount);

        ImGui::Separator();

        //配列の中身表示
        if (ImGui::TreeNode("Stored Targets"))
        {
            int index = 0;
            const glm::vec3 selfPos = Transform().GetWorldPos();

            for (const auto& w : playerInteractableTargets_)
            {
                ImGui::PushID(index);

                if (const auto target = w.lock())
                {
                    const glm::vec3 dir = target->InteractableTransform().GetWorldPos() - selfPos;
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
        if (const auto nearest = CatchInteractTarget().lock())
        {
            const glm::vec3 direction = nearest->InteractableTransform().GetWorldPos() - Transform().GetWorldPos();
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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::InteractableArea);
#pragma endregion

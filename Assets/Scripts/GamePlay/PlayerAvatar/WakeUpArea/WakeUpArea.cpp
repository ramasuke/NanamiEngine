#include "WakeUpArea.h"

#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Core/Game/PlayerAvatar/Wakeable/IPlayerWakeable.h"

namespace GamePlay::PlayerAvatar
{
    std::weak_ptr<IPlayerWakeable>
    WakeUpArea::CatchWakeUpTarget()
    {
        std::shared_ptr<IPlayerWakeable> nearest = nullptr;
        float nearestDistSq = std::numeric_limits<float>::max();

        for (auto it = wakeableTargets_.begin(); it != wakeableTargets_.end();)
        {
            if (const auto target = it->lock())
            {
                if (target->IsDowned())
                {
                    const glm::vec3 direction = target->WakeableTransform().GetWorldPos() - Transform().GetWorldPos();
                    if (const float distSq = glm::dot(direction, direction); distSq < nearestDistSq)
                    {
                        nearestDistSq = distSq;
                        nearest = target;
                    }
                }

                ++it;
            }
            else
            {
                it = wakeableTargets_.erase(it);
            }
        }

        return std::weak_ptr(nearest);
    }

    void WakeUpArea::OnTriggerEnter(
        const Physics::Manifold& contactManifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto playerWakeable = gameObject->Components().Catch<IPlayerWakeable>();
        if (playerWakeable.expired())
            return;

        wakeableTargets_.push_back(playerWakeable);
        playerWakeable.lock()->OnEnterWakeUpRange();
    }

    void WakeUpArea::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        if (!gameObject)
            return;

        const auto leaving = gameObject->Components().Catch<IPlayerWakeable>().lock();
        if (!leaving)
            return;

        std::erase_if(wakeableTargets_, [&](const std::weak_ptr<IPlayerWakeable>& w){
            return w.lock() == leaving;
        });
        leaving->OnExitWakeUpRange();
    }


    void WakeUpArea::OnDrawGui()
    {
        ImGui::TextUnformatted("WakeUp Area");

        int aliveCount   = 0;
        int expiredCount = 0;

        for (const auto& w : wakeableTargets_)
        {
            if (w.expired())
                ++expiredCount;
            else
                ++aliveCount;
        }

        ImGui::Text("Targets: %d (Alive: %d / Expired: %d)",
                    static_cast<int>(wakeableTargets_.size()),
                    aliveCount,
                    expiredCount);

        ImGui::Separator();

        if (const auto nearest = CatchWakeUpTarget().lock())
        {
            const glm::vec3 direction = nearest->WakeableTransform().GetWorldPos() - Transform().GetWorldPos();
            const float dist = glm::length(direction);

            ImGui::Text("Nearest Downed Target");
            ImGui::Text("Distance: %.2f", dist);
        }
        else
        {
            ImGui::TextDisabled("No valid target");
        }
    }
}

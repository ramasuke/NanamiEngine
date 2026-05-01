#include "GenerateParticle.h"

#include "../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GameCore::Npc::Enemy::Behaviour
{

    TickStatus Action::GenerateParticle::DoTick(const TickContext& context)
    {
        const auto enemyPos = context.EnemyTransform().GetWorldPos();
        const auto enemyRot = context.EnemyTransform().GetWorldRot();

        const glm::vec3 rotatedOffset = enemyRot * offset_;

        const glm::vec3 spawnPos =
            isUseAbsolutePosition_
            ? offset_
            : enemyPos + rotatedOffset;

        auto particle = Scene::GameObject::Instantiate(
            particlePrefab_.get(),
            spawnPos
        );

        Coroutine::StartCoroutine(DestroyAfterTimeAsync(particle, lifeTime_));

        return TickStatus::Success;
    }

    void Action::GenerateParticle::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("offset_", offset_);
        ImGuiHelper::OnDrawInputField("lifeTime_", lifeTime_);
        ImGuiHelper::OnDrawInputField("isUseAbsolutePosition_", isUseAbsolutePosition_);
        ImGuiHelper::OnDrawInputField("particlePrefab_", particlePrefab_);
    }

    Coroutine::Task<void> Action::GenerateParticle::DestroyAfterTimeAsync(
        std::weak_ptr<GameObject::IGameObject> particle,
        float lifeTime)
    {
        co_await Coroutine::WaitForSeconds(lifeTime);

        auto p = particle.lock();
        if (!p)
            co_return;

        p->OnDestroy();

        co_return;
    }

}

#include "Enemy_Behaviour_Action_RadiateProjectile.h"

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::RadiateProjectile::DoTick(const TickContext& context)
    {
        const auto enemyRot = context.EnemyTransform().GetWorldRot();

        const glm::quat finalRot = enemyRot;

        const auto projectile = Scene::GameObject::Instantiate(
            *projectilePrefab_.get(),
            spawnPosition_.get(context),
            finalRot
        );

        Coroutine::StartCoroutine(MoveProjectileAsync(context, projectile));

        return TickStatus::Success;
    }

    void Action::RadiateProjectile::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("spawnPosition_" , spawnPosition_);
        ImGuiHelper::OnDrawInputField("targetPosition_", targetPosition_);
    }

    Coroutine::Task<void> Action::RadiateProjectile::MoveProjectileAsync(
        const TickContext context,
        const std::weak_ptr<GameObject::IGameObject>& projectileObject)
    {
        float t = 0.0f;

        auto projectile = projectileObject.lock();
        if (!projectile)
            co_return;

        const glm::vec3 startPos = projectile->Transform().GetWorldPos();

        while (t < 1.0f)
        {
            projectile = projectileObject.lock();
            if (!projectile)
                co_return;

            auto& transform = projectile->Transform();

            const glm::vec3 targetPos = targetPosition_.get(context);

            t += Time::DeltaTime() * moveSpeed_;
            transform.SetWorldPos(glm::mix(startPos, targetPos, t));

            co_await Coroutine::WaitYield();
        }

        if (const auto projectile = projectileObject.lock())
        {
            projectile->Transform().SetWorldPos(targetPosition_.get(context));
        }
    }
}
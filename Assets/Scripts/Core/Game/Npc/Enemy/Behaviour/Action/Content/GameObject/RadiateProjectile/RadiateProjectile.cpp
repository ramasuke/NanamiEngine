#include "RadiateProjectile.h"

#include "../../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/WaitForSeconds.h"
#include "../../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::RadiateProjectile::DoTick(const TickContext& context)
    {
        const auto enemyPos = context.EnemyTransform().GetWorldPos();
        const auto enemyRot = context.EnemyTransform().GetWorldRot();
    
        const glm::vec3 rotatedOffset = enemyRot * instantiateOffsetPos_;
    
        const glm::vec3 spawnPos =
            useAbsoluteSpawnPosition_
            ? instantiateOffsetPos_
            : enemyPos + rotatedOffset;
    
        auto projectile = Scene::GameObject::Instantiate(
            projectilePrefab_.get(),
            spawnPos
        );
    
        Coroutine::StartCoroutine(MoveProjectileAsync(context, projectile));
    
        return TickStatus::Success;
    }
    
    void Action::RadiateProjectile::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("instantiateOffsetPos_", instantiateOffsetPos_);
        ImGuiHelper::OnDrawInputField("projectilePrefab_", projectilePrefab_);
    
        ImGuiHelper::OnDrawInputField("useAbsoluteSpawnPosition_", useAbsoluteSpawnPosition_);
        ImGuiHelper::OnDrawInputField("useAbsoluteTargetPosition_", useAbsoluteTargetPosition_);
    
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
    }
    
    Coroutine::Task<void> Action::RadiateProjectile::MoveProjectileAsync(
        TickContext context,
        const std::weak_ptr<GameObject::IGameObject>& projectileObject)
    {
        auto projectile = projectileObject.lock();
        if (!projectile)
            co_return;
    
        auto& transform = projectile->TransformRef();
    
        const glm::vec3 startPos = transform.GetWorldPos();
    
        const auto enemyPos = context.EnemyTransform().GetWorldPos();
        const auto enemyRot = context.EnemyTransform().GetWorldRot();
    
        const glm::vec3 rotatedOffset = enemyRot * instantiateOffsetPos_;
    
        const glm::vec3 targetPos =
            useAbsoluteTargetPosition_
            ? instantiateOffsetPos_
            : enemyPos + rotatedOffset;
    
        float t = 0.0f;
    
        while (t < 1.0f)
        {
            if (!projectile)
                co_return;
    
            const float dt = 0.016f;
            t += dt * moveSpeed_;
    
            transform.SetWorldPos(glm::mix(startPos, targetPos, t));
    
            co_await Coroutine::WaitForSeconds(std::numeric_limits<float>::min());
        }
    
        transform.SetWorldPos(targetPos);
    
        co_return;
    }
}
#include "RadiateProjectile.h"

#include "../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::RadiateProjectile::DoTick(
        const TickContext& context)
    {
        const auto enemyPos     = context.EnemyTransform().GetWorldPos();
        const auto enemyRot     = context.EnemyTransform().GetWorldRot();
        const auto worldOffset  = enemyRot * instantiateOffsetPos_;
        const auto spawnPos     = enemyPos + worldOffset;
        auto projectile = Scene::GameObject::Instantiate(
            projectilePrefab_.get(),
            spawnPos
        );

        return TickStatus::Success;
    }

    void Action::RadiateProjectile::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("instantiateOffsetPos_", instantiateOffsetPos_);
        ImGuiHelper::OnDrawInputField("projectilePrefab_", projectilePrefab_);
    }

    //Coroutine::Task<void> Action::RadiateProjectile::MoveProjectileAsync(
    //    TickContext context,
    //    const std::weak_ptr<GameObject::IGameObject>& projectileObject)
    //{
    //    co_await  
    //}
}

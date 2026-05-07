#include "Enemy_Behaviour_Action_RadiateProjectile.h"

#include "../../../../../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "../../../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../../../Libs/LibCore/Tween/Ease/Ease.h"

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
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
        ImGuiHelper::OnDrawInputField("projectilePrefab_", projectilePrefab_);
    }

    Coroutine::Task<void> Action::RadiateProjectile::MoveProjectileAsync(
        const TickContext context,
        const std::weak_ptr<GameObject::IGameObject>& projectileObject)
    {
        auto& transform = projectileObject.lock()->Transform();

        const glm::vec3 startPos  = transform.GetWorldPos();
        const glm::vec3 targetPos = targetPosition_.get(context);

        const float distance = glm::distance(startPos, targetPos);
        const float durationSec = distance / moveSpeed_;

        const auto tween = tweeny::from(startPos)
            .to(targetPos)
            .during(static_cast<int>(durationSec * 1000.0f))
            .via(Tween::Ease(EaseType::Linear));

        co_await Coroutine::WaitForTween(transform, tween);
        projectileObject.lock()->OnDestroy();
    }
}

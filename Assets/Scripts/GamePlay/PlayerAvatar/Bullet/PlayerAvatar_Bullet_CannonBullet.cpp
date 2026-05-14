#include "PlayerAvatar_Bullet_CannonBullet.h"

#include "../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Damage/Physics/Game_Damage_Physics.h"
#include "../../../Core/Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"

namespace GamePlay::PlayerAvatar::Bullet
{
    void CannonBullet::OnStart()
    {
        Coroutine::StartCoroutine(ExplosionCountDownAsync());
    }

    void CannonBullet::OnCollisionEnter(
        const Physics::Manifold& maniFold,
        const std::shared_ptr<GameObject::IGameObject>& other)
    {
        for (auto& playerAttackable : other->Components().Catches<GameCore::PlayerAvatar::ITakablePlayerAttack>())
        {
            auto damage = std::make_unique<GameCore::Damage::Physics>(*Entity().lock(), *other, physicsDamage_);
            playerAttackable.lock()->OnTakeDamage(std::move(damage));
        }
        Explosion();
    }
    
    Coroutine::Task<void> CannonBullet::ExplosionCountDownAsync()
    {
        const auto entity = Entity();
        co_await Coroutine::WaitForSeconds(explositionDuration_secs_);
        if (!entity.expired())
        {
            Explosion();
        }
    }

    void CannonBullet::Explosion()
    {
        Scene::GameObject::Instantiate(explosionParticle_.get(), Transform().GetWorldPos());
        Entity().lock()->OnDestroy();
    }

    void CannonBullet::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bulletArriveDuration_secs_", explositionDuration_secs_);
        ImGuiHelper::OnDrawInputField("physicsDamage_", physicsDamage_);
        ImGuiHelper::OnDrawInputField("explosionParticle_", explosionParticle_);
    }
}

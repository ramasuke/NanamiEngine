#include "GamePlay_MagicProjectile.h"

#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../Spawn/GamePlay_PrefabSpawner.h"
#include "../GamePlay_MagicAim.h"

namespace GamePlay::Magic
{
    void MagicProjectile::Launch(const std::weak_ptr<GameObject::IGameObject>& caster,
                                 const GameCore::Damage::PhysicsPower power,
                                 const glm::vec3& velocity,
                                 const float lifeTime_secs)
    {
        caster_                 = caster;
        power_                  = power;
        remainingLifeTime_secs_ = lifeTime_secs;
        isLaunched_             = true;

        if (const auto rigidBody = Components().Catch<Component::RigidBody>().lock())
            rigidBody->SetLinearVelocity(velocity);
    }

    void MagicProjectile::OnUpdate()
    {
        if (!isLaunched_ || hasImpacted_)
            return;

        remainingLifeTime_secs_ -= Time::DeltaTime();
        if (remainingLifeTime_secs_ <= 0.0f)
            Impact();
    }

    void MagicProjectile::OnCollisionEnter(const Physics::Manifold& manifold,
                                           const std::shared_ptr<GameObject::IGameObject>& other)
    {
        if (!isLaunched_ || hasImpacted_)
            return;

        const auto owner = Physics::FindBodyOwner(other);
        if (owner)
        {
            if (owner == caster_.lock())
                return;
            if (!owner->Components().Catch<GameCore::IPlayerAvatar>().expired())
                return;
        }

        if (const auto entity = Entity().lock())
            ApplySpellDamage(*entity, other, power_);
        ShowSpellDamageText(caster_, other, power_, Transform().GetWorldPos());
        Impact();
    }

    void MagicProjectile::Impact()
    {
        hasImpacted_ = true;
        if (impactPrefab_)
            Spawn::SpawnPrefab(*impactPrefab_.get(), Transform().GetWorldPos(), impactLifeTime_secs_);

        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    void MagicProjectile::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("impactPrefab_", impactPrefab_);
        ImGuiHelper::OnDrawInputField("impactLifeTime_secs_", impactLifeTime_secs_);
        ImGui::Text("launched: %s  remaining: %.2f", isLaunched_ ? "true" : "false", remainingLifeTime_secs_);
    }
}

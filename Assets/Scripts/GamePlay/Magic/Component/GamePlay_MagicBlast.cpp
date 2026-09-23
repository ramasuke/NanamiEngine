#include "GamePlay_MagicBlast.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../Core/Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"
#include "../../Spawn/GamePlay_PrefabSpawner.h"
#include "../GamePlay_MagicAim.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Magic
{
    void MagicBlast::Arm(const std::weak_ptr<GameObject::IGameObject>& caster,
                         const GameCore::Damage::PhysicsPower power,
                         const float delay_secs)
    {
        caster_              = caster;
        power_               = power;
        remainingDelay_secs_ = delay_secs;
        isArmed_             = true;
    }

    void MagicBlast::OnUpdate()
    {
        if (!isArmed_ || hasDetonated_)
            return;

        if (detonateOnEnter_)
        {
            if (HasLiveTarget())
                Detonate();
            return;
        }

        remainingDelay_secs_ -= Time::DeltaTime();
        if (remainingDelay_secs_ <= 0.0f)
            Detonate();
    }

    void MagicBlast::OnTriggerEnter(const Physics::Manifold& manifold, const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto owner = Physics::FindBodyOwner(gameObject);
        if (!owner || owner == caster_.lock())
            return;
        if (owner->Components().Catch<GameCore::PlayerAvatar::ITakablePlayerAttack>().expired())
            return;

        const bool isKnown = std::ranges::any_of(targets_, [&](const BlastTarget& target) { return target.owner.lock() == owner; });
        if (!isKnown)
            targets_.push_back(BlastTarget{ owner, gameObject });
    }

    void MagicBlast::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        std::erase_if(targets_, [&](const BlastTarget& target)
        {
            const auto part = target.part.lock();
            return !part || part == gameObject;
        });
    }

    bool MagicBlast::HasLiveTarget() const
    {
        return std::ranges::any_of(targets_, [](const BlastTarget& target) { return !target.owner.expired(); });
    }

    void MagicBlast::Detonate()
    {
        hasDetonated_ = true;

        if (const auto entity = Entity().lock())
        {
            bool hasShaken = false;
            for (const auto& target : targets_)
            {
                const auto part = target.part.lock();
                if (!part)
                    continue;

                ApplySpellDamage(*entity, part, power_);
                ShowSpellDamageText(caster_, part, power_, HitPartPosition(*part));
                // NOTE: 揺れは重なるので、何体巻き込んでも 1 回だけ
                if (!hasShaken)
                    hasShaken = ShakeOnSpellHit(caster_, part, hitShakeIntensity_, hitShakeDuration_secs_);
            }
        }

        if (detonatePrefab_)
            Spawn::SpawnPrefab(*detonatePrefab_.get(), Transform().GetWorldPos(), detonateEffectLifeTime_secs_);

        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    void MagicBlast::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("detonatePrefab_", detonatePrefab_);
        ImGuiHelper::OnDrawInputField("detonateEffectLifeTime_secs_", detonateEffectLifeTime_secs_);
        ImGuiHelper::OnDrawInputField("detonateOnEnter_", detonateOnEnter_);
        ImGuiHelper::OnDrawInputField("hitShakeIntensity_", hitShakeIntensity_);
        ImGuiHelper::OnDrawInputField("hitShakeDuration_secs_", hitShakeDuration_secs_);
        ImGui::Text("armed: %s  targets: %d", isArmed_ ? "true" : "false", static_cast<int>(targets_.size()));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Magic::MagicBlast);
#pragma endregion

#include "GamePlay_MagicChannel.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../Core/Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"
#include "../../Spawn/GamePlay_PrefabSpawner.h"
#include "../GamePlay_MagicAim.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Magic
{
    void MagicChannel::Begin(const std::weak_ptr<GameObject::IGameObject>& caster,
                             const GameCore::Damage::PhysicsPower powerPerTick,
                             const float duration_secs,
                             const float tickInterval_secs)
    {
        caster_                 = caster;
        powerPerTick_           = powerPerTick;
        remainingDuration_secs_ = duration_secs;
        tickInterval_secs_      = (std::max)(tickInterval_secs, 0.05f);
        // センサーの接触は次の物理ステップで入るので、最初の1回も1間隔待ってから当てる
        untilNextTick_secs_     = tickInterval_secs_;
        isChanneling_           = true;
        hasBegun_               = true;
    }

    void MagicChannel::OnUpdate()
    {
        if (!hasBegun_)
            return;

        const float deltaTime = Time::DeltaTime();
        remainingDuration_secs_ -= deltaTime;

        if (isChanneling_)
        {
            untilNextTick_secs_ -= deltaTime;
            if (untilNextTick_secs_ <= 0.0f)
            {
                untilNextTick_secs_ += tickInterval_secs_;
                ApplyTick();
            }
            if (remainingDuration_secs_ <= 0.0f)
                isChanneling_ = false;
        }

        if (remainingDuration_secs_ <= -linger_secs_)
        {
            hasBegun_ = false;
            if (const auto entity = Entity().lock())
                entity->OnDestroy();
        }
    }

    void MagicChannel::OnTriggerEnter(const Physics::Manifold& manifold, const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto owner = Physics::FindBodyOwner(gameObject);
        if (!owner || owner == caster_.lock())
            return;
        if (owner->Components().Catch<GameCore::PlayerAvatar::ITakablePlayerAttack>().expired())
            return;

        const bool isKnown = std::ranges::any_of(targets_, [&](const ChannelTarget& target) { return target.owner.lock() == owner; });
        if (!isKnown)
            targets_.push_back(ChannelTarget{ owner, gameObject });
    }

    void MagicChannel::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        std::erase_if(targets_, [&](const ChannelTarget& target)
        {
            const auto part = target.part.lock();
            return !part || part == gameObject;
        });
    }

    void MagicChannel::ApplyTick()
    {
        const auto entity = Entity().lock();
        if (!entity)
            return;

        for (const auto& target : targets_)
        {
            const auto part = target.part.lock();
            if (!part || target.owner.expired())
                continue;

            const glm::vec3 hitPos = HitPartPosition(*part);
            ApplySpellDamage(*entity, part, powerPerTick_);
            ShowSpellDamageText(caster_, part, powerPerTick_, hitPos);

            if (hitPrefab_)
                Spawn::SpawnPrefab(*hitPrefab_.get(), hitPos, hitEffectLifeTime_secs_);
        }
    }

    void MagicChannel::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("hitPrefab_", hitPrefab_);
        ImGuiHelper::OnDrawInputField("hitEffectLifeTime_secs_", hitEffectLifeTime_secs_);
        ImGuiHelper::OnDrawInputField("linger_secs_", linger_secs_);
        ImGui::Text("channeling: %s  remaining: %.2f  targets: %d",
                    isChanneling_ ? "true" : "false", remainingDuration_secs_, static_cast<int>(targets_.size()));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Magic::MagicChannel);
#pragma endregion

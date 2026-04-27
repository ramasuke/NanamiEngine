#pragma once
#include <../cereal/include/cereal/types/vector.hpp>

#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class GenerateParticle final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        Coroutine::Task<void> DestroyAfterTimeAsync(
            std::weak_ptr<GameObject::IGameObject> particle,
            float lifeTime);

    private:
        [[serialize(0)]] glm::vec3 offset_ = {};
        [[serialize(0)]] float lifeTime_ = 1.5f;
        [[serialize(0)]] bool isUseAbsolutePosition_ = false;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) particlePrefab_;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(lifeTime_));
            archive(CEREAL_NVP(isUseAbsolutePosition_));
            archive(CEREAL_NVP(particlePrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(offset_));
            if (version >= 0) archive(CEREAL_NVP(lifeTime_));
            if (version >= 0) archive(CEREAL_NVP(isUseAbsolutePosition_));
            if (version >= 0) archive(CEREAL_NVP(particlePrefab_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(GenerateParticle, "Particle::GenerateParticle")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::GenerateParticle, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::GenerateParticle)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::GenerateParticle)
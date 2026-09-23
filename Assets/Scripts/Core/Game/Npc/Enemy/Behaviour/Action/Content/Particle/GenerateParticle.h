#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class GenerateParticle final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

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

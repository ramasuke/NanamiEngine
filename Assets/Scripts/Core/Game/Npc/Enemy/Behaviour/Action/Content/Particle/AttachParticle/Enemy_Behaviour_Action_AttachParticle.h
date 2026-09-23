#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * position_(ワールド座標)にパーティクルを出し、target_ に貼り付けたまま動かす。
     * 燃えている島が落ちるとき、炎と煙も一緒に落ちるようにするためのもの。
     */
    class AttachParticle final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] glm::vec3 position_ = {};
        [[serialize(0)]] float lifeTime_ = 0.0f;
        [[serialize(0)]] FIELD(GameObject::IGameObject) target_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) particlePrefab_;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(position_));
            archive(CEREAL_NVP(lifeTime_));
            archive(CEREAL_NVP(target_));
            archive(CEREAL_NVP(particlePrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(position_));
            if (version >= 0) archive(CEREAL_NVP(lifeTime_));
            if (version >= 0) archive(CEREAL_NVP(target_));
            if (version >= 0) archive(CEREAL_NVP(particlePrefab_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(AttachParticle, "Particle::AttachParticle")
}

#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class Random final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        int successRate_ = 50; // 0〜100 (%)

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(successRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(successRate_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION(Random)
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::Random, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::Random)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::Random
)

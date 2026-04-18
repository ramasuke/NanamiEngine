#pragma once
#include "../../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ConditionArriveDuration final : public ActionBase
    {
    public:
        TickStatus DoTick(const TickContext& context) override;

    private:
        [[serialize(0)]] float arriveDuration_secs_ = 0.0f;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(arriveDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(arriveDuration_secs_));
        }

#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ConditionArriveDuration, "EnemyStatus::Condition::ArriveDuration")
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ConditionArriveDuration)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ConditionArriveDuration)
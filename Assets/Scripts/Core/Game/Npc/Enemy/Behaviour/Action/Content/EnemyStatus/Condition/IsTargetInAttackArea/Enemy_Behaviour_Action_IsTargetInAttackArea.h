#pragma once
#include "../../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class IsTargetInAttackArea final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] std::string attackAreaName_;
        [[serialize(1)]] bool isInner_ = true;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(attackAreaName_));
            archive(CEREAL_NVP(isInner_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(attackAreaName_));
            if (version >= 1) archive(CEREAL_NVP(isInner_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(IsTargetInAttackArea, "EnemyStatus::Condition::IsTargetInAttackArea")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::IsTargetInAttackArea, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::IsTargetInAttackArea)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::IsTargetInAttackArea)

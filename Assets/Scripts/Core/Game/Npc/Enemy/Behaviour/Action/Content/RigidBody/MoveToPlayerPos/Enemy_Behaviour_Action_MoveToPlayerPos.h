#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class MoveToPlayerPos final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] float moveSpeed_         = 0.0f;
        [[serialize(0)]] float moveStartDistance_ = 0.0f;
        [[serialize(0)]] int   animationNumber_   = -1;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(moveStartDistance_));
            archive(CEREAL_NVP(animationNumber_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(moveStartDistance_));
            if (version >= 0) archive(CEREAL_NVP(animationNumber_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION_WITH_NAME(MoveToPlayerPos, "RigidBody::MoveToePlayerPos")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::MoveToPlayerPos, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::MoveToPlayerPos)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::MoveToPlayerPos
)

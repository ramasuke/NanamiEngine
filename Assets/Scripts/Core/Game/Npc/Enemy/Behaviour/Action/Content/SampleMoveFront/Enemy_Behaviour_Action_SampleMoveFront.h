#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class SampleMoveFront final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION(SampleMoveFront)
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::SampleMoveFront)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::SampleMoveFront)
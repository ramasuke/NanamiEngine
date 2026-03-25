#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class AddRotation final : public ActionBase 
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

    private:
        float yawStep_degree_ = 5.0f;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(yawStep_degree_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(yawStep_degree_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION(AddRotation)
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::AddRotation)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::AddRotation)

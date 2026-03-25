#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class WriteBlackBoard final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] std::string keyName_;
        [[serialize(0)]] int value_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(keyName_));
            archive(CEREAL_NVP(value_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(keyName_));
            archive(CEREAL_NVP(value_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION(WriteBlackBoard)
}
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::WriteBlackBoard, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::WriteBlackBoard)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::WriteBlackBoard)

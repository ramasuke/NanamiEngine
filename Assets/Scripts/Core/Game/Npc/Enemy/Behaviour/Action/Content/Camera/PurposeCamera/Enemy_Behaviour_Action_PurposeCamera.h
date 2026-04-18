#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class PurposeCamera final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        
        [[serialize(0)]] std::string prefabPurposeCamera_;
        [[serialize(1)]] int priority_ = 100;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(prefabPurposeCamera_));
            archive(CEREAL_NVP(priority_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(prefabPurposeCamera_));
            if (version >= 1) archive(CEREAL_NVP(priority_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(PurposeCamera, "Camera::PurposeCamera")
}
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::PurposeCamera, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PurposeCamera)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::PurposeCamera)
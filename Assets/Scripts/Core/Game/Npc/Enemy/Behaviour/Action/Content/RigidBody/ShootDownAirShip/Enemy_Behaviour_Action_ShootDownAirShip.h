#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../../../GamePlay/Prop/AirShip/Prop_AirShip.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ShootDownAirShip final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;


        FIELD(GamePlay::Prop::AirShip) shootDownAirShip_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(shootDownAirShip_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(shootDownAirShip_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(ShootDownAirShip, "RigidBody::Other::ShootDownAirShip")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ShootDownAirShip, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ShootDownAirShip)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ShootDownAirShip)
#pragma once
#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_Constraints.h"
#include "../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class SetFreezePhysics final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;


        [[serialize(0)]] Physics::Constraints constraints_ = Physics::Constraints::None;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(constraints_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(constraints_));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(SetFreezePhysics, "NpcStatus::RigidBody::SetFreezePhysics")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::SetFreezePhysics, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SetFreezePhysics)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::SetFreezePhysics)
#pragma once
#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Data/FriendlyNpcWalkingRoute/Data_NpcWalkingRoute.h"
#include "../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class SetLinearVelocity final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        
        [[serialize(1)]] glm::vec3 setVelocity_ = glm::vec3{0, 0, 0};

#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(setVelocity_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 1) archive(CEREAL_NVP(setVelocity_));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(SetLinearVelocity, "NpcStatus::RigidBody::SetLinearVelocity")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::SetLinearVelocity, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SetLinearVelocity)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Friendly::Behaviour::ActionBase,
    GameCore::Npc::Friendly::Behaviour::Action::SetLinearVelocity)
#pragma once
#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Data/FriendlyNpcWalkingRoute/Data_NpcWalkingRoute.h"
#include "../../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../../../Other/WriteBlackBoard/Friendly_Behaviour_Action_WriteBlackBoard.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class MoveForRoute final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        FIELD(Asset::NpcWalkingRoute) moveRoute_;
        float moveSpeed_                = 0.0f;
        float turnRotateSpeed_          = 0.0f;
        int   currentRouteIndex_        = 0;

        static constexpr float ARRIVE_THRESHOLD = 10.0f;

#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(moveRoute_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(turnRotateSpeed_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(moveRoute_));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(turnRotateSpeed_));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(MoveForRoute, "NpcStatus::RigidBody::MoveForRoute")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::MoveForRoute, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::MoveForRoute)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Friendly::Behaviour::ActionBase,
    GameCore::Npc::Friendly::Behaviour::Action::MoveForRoute)
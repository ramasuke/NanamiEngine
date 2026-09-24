#pragma once
#include "../../../../Friendly_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
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
        [[serialize(1)]] float arriveDistance_ = 10.0f;   // 経由点にこの距離まで近づいたら次へ
        int   currentRouteIndex_        = 0;

#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(moveRoute_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(turnRotateSpeed_));
            archive(CEREAL_NVP(arriveDistance_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(moveRoute_));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(turnRotateSpeed_));
            if (version >= 1) archive(CEREAL_NVP(arriveDistance_));
        }
#pragma endregion
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(MoveForRoute, "NpcStatus::RigidBody::MoveForRoute")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::MoveForRoute, 1)

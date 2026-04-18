#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Data/EventNpcWalkingRoute/Data_EventNpcWalkingRoute.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace Data::EventNpcWalkingRoute
{
    struct RoutePoint;
}

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class MoveEventRoute final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        Coroutine::Task<void> MoveAsync(TickContext context);
        Coroutine::Task<void> TweenAsync(
            const Data::EventNpcWalkingRoute::RoutePoint& throughRoute,
            TickContext context
        );

        void DoDrawGui() override;

        FIELD(Asset::EventNpcWalkingRoute) moveRoute_;

        bool  isOnceExecute_     = false;
        bool  isExecuted_        = false;
        bool  isRotateToMoveDir_ = true;
        float rotateSpeedDeg_   = 180.0f;

        bool isRunning_  = false;
        bool isFinished_ = false;

        int  currentRouteIndex_ = 0;

#pragma region Serialization
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(moveRoute_));
            archive(CEREAL_NVP(isOnceExecute_));
            archive(CEREAL_NVP(isRotateToMoveDir_));
            archive(CEREAL_NVP(rotateSpeedDeg_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(moveRoute_));
            if (version >= 0) archive(CEREAL_NVP(isOnceExecute_));
            if (version >= 1) archive(CEREAL_NVP(isRotateToMoveDir_));
            if (version >= 1) archive(CEREAL_NVP(rotateSpeedDeg_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(MoveEventRoute, "RigidBody::MoveEventRoute")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::MoveEventRoute, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::MoveEventRoute)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase,
                                     GameCore::Npc::Enemy::Behaviour::Action::MoveEventRoute)

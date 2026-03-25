#pragma once
#include "../../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../Data/EventNpcWalkingRoute/Data_EventNpcWalkingRoute.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../Behaviour/Action/Enemy_Behaviour_ActionBase.h"

namespace GameCore::Npc::Enemy::FirstEventDragon
{
    class AppearToTouchDownIsLand final : public Behaviour::ActionBase
    {
        Behaviour::TickStatus DoTick(const Behaviour::Action::TickContext& context) override;
        Coroutine::Task<void> AppearToTouchDownIsLandAsync(Behaviour::Action::TickContext context);
        Coroutine::Task<void> ToDestroyAirShipMovieAsync  (Behaviour::Action::TickContext context);
        Coroutine::Task<void> ToTouchDownIsLandAsync      (Behaviour::Action::TickContext context);
        Coroutine::Task<void> StartTween(const Data::EventNpcWalkingRoute::RoutePoint& throughRoute, const Behaviour::Action::TickContext context);
        void DoDrawGui() override;

        FIELD(Asset::EventNpcWalkingRoute) toDestroyAirShipRoute_;
        FIELD(Asset::EventNpcWalkingRoute) toTouchDownIsLandRoute_;
        bool isRunning_  = false;
        bool isFinished_ = false;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(toDestroyAirShipRoute_));
            archive(CEREAL_NVP(toTouchDownIsLandRoute_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(toDestroyAirShipRoute_));
            if (version >= 0) archive(CEREAL_NVP(toTouchDownIsLandRoute_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION(AppearToTouchDownIsLand)
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::FirstEventDragon::AppearToTouchDownIsLand, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::FirstEventDragon::AppearToTouchDownIsLand)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::FirstEventDragon::AppearToTouchDownIsLand)
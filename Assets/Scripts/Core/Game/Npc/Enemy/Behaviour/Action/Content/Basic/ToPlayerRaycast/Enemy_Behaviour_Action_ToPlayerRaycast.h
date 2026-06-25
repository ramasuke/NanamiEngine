#pragma once
#include <vector>
#include "cereal/types/vector.hpp"
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ToPlayerRaycast final : public ActionBase
    {
    private:
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] std::vector<Physics::Layer> layers_      = { Physics::Layer::Enemy };
        [[serialize(1)]] float                       maxDistance_ = 10.0f;
        [[serialize(2)]] float                       offsetY_     = 10.0f;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(layers_));
            archive(CEREAL_NVP(maxDistance_));
            archive(CEREAL_NVP(offsetY_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(layers_));
            if (version >= 1) archive(CEREAL_NVP(maxDistance_));
            if (version >= 2) archive(CEREAL_NVP(offsetY_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ToPlayerRaycast, "Basic::ToPlayerRaycast")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ToPlayerRaycast, 2)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ToPlayerRaycast)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::ToPlayerRaycast
)

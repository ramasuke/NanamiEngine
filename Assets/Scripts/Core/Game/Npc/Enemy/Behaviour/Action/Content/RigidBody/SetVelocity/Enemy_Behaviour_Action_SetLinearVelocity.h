#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class SetLinearVelocity final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        
        glm::vec3 velocity_ = {};
        bool isGravity_ = true;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(velocity_));
            archive(CEREAL_NVP(isGravity_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(velocity_));
            if (version >= 1) archive(CEREAL_NVP(isGravity_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(SetLinearVelocity, "RigidBody::SetLinearVelocity")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::SetLinearVelocity, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::SetLinearVelocity)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::SetLinearVelocity)
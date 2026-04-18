#pragma once
#include <../cereal/include/cereal/types/vector.hpp>
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Component/Collider/ColliderBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ChangeColliderEmotionType final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        std::vector<FIELD(Component::ColliderBase)> colliders_;
        JPH::EMotionType emotionType_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(colliders_));
            archive(CEREAL_NVP(emotionType_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(colliders_));
            archive(CEREAL_NVP(emotionType_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION_WITH_NAME(ChangeColliderEmotionType, "RigidBody::ChangeEmotionType")
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeColliderEmotionType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ChangeColliderEmotionType)

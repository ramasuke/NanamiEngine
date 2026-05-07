#pragma once
#include <../cereal/include/cereal/types/vector.hpp>
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Component/Collider/ColliderBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../FieldGameObject/Enemy_Behaviour_Action_FieldGameObject.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class ChangeColliderEmotionType final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] JPH::EMotionType emotionType_ = JPH::EMotionType::Dynamic;
        [[serialize(0)]] std::vector<FieldGameObject<Component::ColliderBase>> colliders_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(emotionType_));
            archive(CEREAL_NVP(colliders_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(emotionType_));
            if (version >= 0) archive(CEREAL_NVP(colliders_));
        }
#pragma endregion
    };
    
    REGISTER_ENEMY_ACTION_WITH_NAME(ChangeColliderEmotionType, "RigidBody::ChangeEmotionType")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ChangeColliderEmotionType, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeColliderEmotionType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ChangeColliderEmotionType)

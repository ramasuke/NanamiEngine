#pragma once
#include "../../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class RotateOnPlayer final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        float rotateSpeed_     = 180.0f;
        float toleranceDeg_    = 5.0f;
        float offsetRotateDeg_ = 0.0f;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(toleranceDeg_));
            archive(CEREAL_NVP(offsetRotateDeg_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(toleranceDeg_));
            if (version >= 1) archive(CEREAL_NVP(offsetRotateDeg_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(RotateOnPlayer, "Transform::Rotation::RotateOnPlayer")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::RotateOnPlayer, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::RotateOnPlayer)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::RotateOnPlayer
)

#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class PlayAnimation final : public ActionBase
    {
        
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;
        
        [[serialize(0)]] int animatorSetParamNumber_ = 0;
        [[serialize(1)]] FIELD(Asset::SoundFile) animationSound_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(animatorSetParamNumber_));
            archive(CEREAL_NVP(animationSound_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(animatorSetParamNumber_));
            if (version >= 1) archive(CEREAL_NVP(animationSound_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(PlayAnimation, "EnemyStatus::PlayAnimation")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::PlayAnimation, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PlayAnimation)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::PlayAnimation)
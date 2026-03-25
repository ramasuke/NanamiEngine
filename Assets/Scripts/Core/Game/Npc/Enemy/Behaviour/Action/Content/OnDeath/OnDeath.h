#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class OnDeath final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        int animatorSetParam_ = 0;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(animatorSetParam_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(animatorSetParam_);
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION(OnDeath)
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::OnDeath, 0)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::OnDeath)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::OnDeath)

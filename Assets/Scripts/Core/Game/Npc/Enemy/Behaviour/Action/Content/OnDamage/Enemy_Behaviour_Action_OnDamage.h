#pragma once
#include "../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class OnDamage final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;

        FIELD(Asset::PrefabGameObjectFile) damageEffectPrefab_;
        glm::vec3 damageEffectOffset_ = glm::vec3(0.0f);
        int animatorSetParam_ = 0;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(damageEffectPrefab_);
            archive(damageEffectOffset_);
            archive(animatorSetParam_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            archive(damageEffectPrefab_);
            archive(damageEffectOffset_);
            if (version >= 1) archive(animatorSetParam_);
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION(OnDamage)
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::OnDamage, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::OnDamage)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::OnDamage)

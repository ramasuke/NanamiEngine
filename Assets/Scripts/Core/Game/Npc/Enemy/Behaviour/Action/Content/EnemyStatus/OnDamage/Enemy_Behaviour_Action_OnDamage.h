#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../LibCore/cereal/glm/GlmHelper.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class OnDamage final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        [[nodiscard]] bool IsStunned(const TickContext& context) const;
        /** @brief このダメージで立てるスタンの値。立てないなら 0 */
        [[nodiscard]] int ResolveStunStateValue(const TickContext& context, const IDamage& damage, int rawDamage) const;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) damageEffectPrefab_;
        [[serialize(0)]] glm::vec3 damageEffectOffset_ = glm::vec3(0.0f);
        [[serialize(1)]] int animatorSetParam_ = 0;
        [[serialize(2)]] bool isOnDamagedReturnBehaviour_ = false;
        [[serialize(3)]] float knockbackForcePerDamage_ = 0.0f;
        [[serialize(4)]] std::string stunStateKeyName_;
        [[serialize(4)]] float stunnedDamageScale_ = 1.0f;
        // NOTE: 黒板の値で溜めカウンターと部位破壊(脚など)のスタンを別の枝に振り分ける
        [[serialize(5)]] int chargeCounterStunStateValue_ = 1;
        [[serialize(5)]] int breakStunStateValue_ = 1;

#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(damageEffectPrefab_);
            archive(damageEffectOffset_);
            archive(animatorSetParam_);
            archive(isOnDamagedReturnBehaviour_);
            archive(knockbackForcePerDamage_);
            archive(stunStateKeyName_);
            archive(stunnedDamageScale_);
            archive(chargeCounterStunStateValue_);
            archive(breakStunStateValue_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            archive(damageEffectPrefab_);
            archive(damageEffectOffset_);
            if (version >= 1) archive(animatorSetParam_);
            if (version >= 2) archive(isOnDamagedReturnBehaviour_);
            if (version >= 3) archive(knockbackForcePerDamage_);
            if (version >= 4) archive(stunStateKeyName_);
            if (version >= 4) archive(stunnedDamageScale_);
            if (version >= 5) archive(chargeCounterStunStateValue_);
            if (version >= 5) archive(breakStunStateValue_);
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(OnDamage, "EnemyStatus::OnDamage")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::OnDamage, 5)

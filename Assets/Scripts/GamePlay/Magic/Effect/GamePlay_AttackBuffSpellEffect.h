#pragma once
#include "cereal/archives/json.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Core/Game/Magic/IMagicSpellEffect.h"
#include "../../../Core/Game/Magic/MagicSpellEffectFactory.h"

namespace GamePlay::Magic
{
    // 撃ち手のまわりにいる自分と仲間の攻撃力を、しばらく倍率で上げる
    class AttackBuffSpellEffect final : public GameCore::Magic::IMagicSpellEffect
    {
    public:
        [[nodiscard]] GameCore::Magic::MagicCastTarget Aim(const GameCore::Magic::IMagicCaster& caster) const override;
        void Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const override;

    private:
        [[serialize(0)]] float rate_ = 1.3f;
        [[serialize(0)]] float duration_secs_ = 20.0f;
        [[serialize(0)]] float radius_ = 60.0f;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) effectPrefab_;
        [[serialize(0)]] float effectLifeTime_secs_ = 2.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            archive(CEREAL_NVP(rate_));
            archive(CEREAL_NVP(duration_secs_));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(effectPrefab_));
            archive(CEREAL_NVP(effectLifeTime_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            if (version >= 0) archive(CEREAL_NVP(rate_));
            if (version >= 0) archive(CEREAL_NVP(duration_secs_));
            if (version >= 0) archive(CEREAL_NVP(radius_));
            if (version >= 0) archive(CEREAL_NVP(effectPrefab_));
            if (version >= 0) archive(CEREAL_NVP(effectLifeTime_secs_));
        }
#pragma endregion
    };

    REGISTER_MAGIC_SPELL_EFFECT(AttackBuffSpellEffect)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Magic::AttackBuffSpellEffect, 0);
CEREAL_REGISTER_TYPE(GamePlay::Magic::AttackBuffSpellEffect);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Magic::IMagicSpellEffect, GamePlay::Magic::AttackBuffSpellEffect);
#pragma endregion

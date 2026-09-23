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
    // 撃ち手のまわりにいる自分と仲間の体力を戻す
    class HealSpellEffect final : public GameCore::Magic::IMagicSpellEffect
    {
    public:
        [[nodiscard]] GameCore::Magic::MagicCastTarget Aim(const GameCore::Magic::IMagicCaster& caster) const override;
        void Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const override;

    private:
        [[serialize(0)]] int   amount_ = 30;
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
            archive(CEREAL_NVP(amount_));
            archive(CEREAL_NVP(radius_));
            archive(CEREAL_NVP(effectPrefab_));
            archive(CEREAL_NVP(effectLifeTime_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            if (version >= 0) archive(CEREAL_NVP(amount_));
            if (version >= 0) archive(CEREAL_NVP(radius_));
            if (version >= 0) archive(CEREAL_NVP(effectPrefab_));
            if (version >= 0) archive(CEREAL_NVP(effectLifeTime_secs_));
        }
#pragma endregion
    };

    REGISTER_MAGIC_SPELL_EFFECT(HealSpellEffect)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Magic::HealSpellEffect, 0);
#pragma endregion

#pragma once
#include "cereal/archives/json.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../../Core/Game/Magic/IMagicSpellEffect.h"
#include "../../../Core/Game/Magic/MagicSpellEffectFactory.h"

namespace GamePlay::Magic
{
    // 狙った地点の足元に、少し遅れて範囲攻撃を起こす
    class AreaSpellEffect final : public GameCore::Magic::IMagicSpellEffect
    {
    public:
        [[nodiscard]] GameCore::Magic::MagicCastTarget Aim(const GameCore::Magic::IMagicCaster& caster) const override;
        void Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const override;

    private:
        /** @brief センサーの SphereCollider と MagicBlast を持つプレハブ */
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) blastPrefab_;
        [[serialize(0)]] GameCore::Damage::PhysicsPower power_ = GameCore::Damage::PhysicsPower(20);
        [[serialize(0)]] float delay_secs_ = 0.5f;
        /** @brief ロックオンしていない時に狙う距離 */
        [[serialize(0)]] float range_ = 150.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            archive(CEREAL_NVP(blastPrefab_));
            archive(CEREAL_NVP(power_));
            archive(CEREAL_NVP(delay_secs_));
            archive(CEREAL_NVP(range_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            if (version >= 0) archive(CEREAL_NVP(blastPrefab_));
            if (version >= 0) archive(CEREAL_NVP(power_));
            if (version >= 0) archive(CEREAL_NVP(delay_secs_));
            if (version >= 0) archive(CEREAL_NVP(range_));
        }
#pragma endregion
    };

    REGISTER_MAGIC_SPELL_EFFECT(AreaSpellEffect)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Magic::AreaSpellEffect, 0);
#pragma endregion

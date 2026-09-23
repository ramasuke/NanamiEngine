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
    // 撃ち手の前（ロックオン中は対象の方向）の地面に、壁や罠をしばらく置く
    class PlacementSpellEffect final : public GameCore::Magic::IMagicSpellEffect
    {
    public:
        [[nodiscard]] GameCore::Magic::MagicCastTarget Aim(const GameCore::Magic::IMagicCaster& caster) const override;
        void Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const override;

    private:
        /** @brief MagicPlacement を持つプレハブ。罠は MagicBlast（detonateOnEnter_）も持つ */
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) placementPrefab_;
        /** @brief 罠が起爆した時の威力。壁では使わない */
        [[serialize(0)]] GameCore::Damage::PhysicsPower power_ = GameCore::Damage::PhysicsPower(0);
        [[serialize(0)]] float lifeTime_secs_ = 10.0f;
        /** @brief 撃ち手からどれだけ前に置くか */
        [[serialize(0)]] float distance_ = 40.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            archive(CEREAL_NVP(placementPrefab_));
            archive(CEREAL_NVP(power_));
            archive(CEREAL_NVP(lifeTime_secs_));
            archive(CEREAL_NVP(distance_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            if (version >= 0) archive(CEREAL_NVP(placementPrefab_));
            if (version >= 0) archive(CEREAL_NVP(power_));
            if (version >= 0) archive(CEREAL_NVP(lifeTime_secs_));
            if (version >= 0) archive(CEREAL_NVP(distance_));
        }
#pragma endregion
    };

    REGISTER_MAGIC_SPELL_EFFECT(PlacementSpellEffect)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Magic::PlacementSpellEffect, 0);
#pragma endregion

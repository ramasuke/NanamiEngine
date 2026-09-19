#pragma once
#include "cereal/archives/json.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Core/Game/Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../../Core/Game/Magic/IMagicSpellEffect.h"
#include "../../../Core/Game/Magic/MagicSpellEffectFactory.h"

namespace GamePlay::Magic
{
    // 撃ち手の手元から狙った方向へ、光線や吐息をしばらく出し続ける
    class ChannelSpellEffect final : public GameCore::Magic::IMagicSpellEffect
    {
    public:
        [[nodiscard]] GameCore::Magic::MagicCastTarget Aim(const GameCore::Magic::IMagicCaster& caster) const override;
        void Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const override;

    private:
        /** @brief MagicChannel と、-Z 方向へ伸びたセンサーを持つプレハブ */
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) channelPrefab_;
        /** @brief 1回当てるごとの威力 */
        [[serialize(0)]] GameCore::Damage::PhysicsPower power_ = GameCore::Damage::PhysicsPower(6);
        [[serialize(0)]] float duration_secs_     = 1.5f;
        [[serialize(0)]] float tickInterval_secs_ = 0.25f;
        /** @brief ロックオンしていない時に狙う距離 */
        [[serialize(0)]] float range_ = 150.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            archive(CEREAL_NVP(channelPrefab_));
            archive(CEREAL_NVP(power_));
            archive(CEREAL_NVP(duration_secs_));
            archive(CEREAL_NVP(tickInterval_secs_));
            archive(CEREAL_NVP(range_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IMagicSpellEffect>(this));
            if (version >= 0) archive(CEREAL_NVP(channelPrefab_));
            if (version >= 0) archive(CEREAL_NVP(power_));
            if (version >= 0) archive(CEREAL_NVP(duration_secs_));
            if (version >= 0) archive(CEREAL_NVP(tickInterval_secs_));
            if (version >= 0) archive(CEREAL_NVP(range_));
        }
#pragma endregion
    };

    REGISTER_MAGIC_SPELL_EFFECT(ChannelSpellEffect)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Magic::ChannelSpellEffect, 0);
CEREAL_REGISTER_TYPE(GamePlay::Magic::ChannelSpellEffect);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Magic::IMagicSpellEffect, GamePlay::Magic::ChannelSpellEffect);
#pragma endregion

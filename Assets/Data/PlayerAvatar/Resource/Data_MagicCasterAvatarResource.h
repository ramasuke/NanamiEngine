#pragma once
#include <array>
#include <string>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Magic/Data_MagicSpellData.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto MAGIC_CASTER_RESOURCE_EXTENSION_LABEL = ".magicCasterResource";

    class MagicCasterAvatarResource final : public ScriptableObject
    {
    public:
        /** 持ち込める魔法の枠数。LT で1ページ目の4つ、LT+RB で2ページ目の4つ */
        static constexpr int LOADOUT_SLOT_COUNT = 8;

        explicit MagicCasterAvatarResource(const std::string& contentPath = "");
        /** @brief RT / 左クリックで撃つ魔法 */
        [[nodiscard]] std::shared_ptr<const GameCore::Magic::IMagicSpell> BasicSpell() const { return basicSpell_.get(); }
        /** @brief 持ち込んだ魔法。枠が空なら nullptr */
        [[nodiscard]] std::shared_ptr<const GameCore::Magic::IMagicSpell> LoadoutSpell(int slot) const;
        [[nodiscard]] float GroundCheckRadius  () const { return groundCheckRadius_;   }
        [[nodiscard]] float GroundCheckUpOffset() const { return groundCheckUpOffset_; }
        [[nodiscard]] float GroundCheckDistance() const { return groundCheckDistance_; }
        /** 歩き・走りで登れる斜面の最大角度。これより急な面へ向かう速度は消す */
        [[nodiscard]] float MaxWalkableSlope_deg() const { return maxWalkableSlope_deg_; }
        /** 斜面判定SphereCastの半径。カプセルの半径より少し小さくする */
        [[nodiscard]] float SlopeCheckRadius    () const { return slopeCheckRadius_;     }
        /** 斜面判定SphereCastの球の下端を足元からどれだけ上に置くか。これより低い段差は判定に掛からない */
        [[nodiscard]] float SlopeCheckUpOffset  () const { return slopeCheckUpOffset_;   }
        /** 斜面判定SphereCastの進行方向への探索距離 */
        [[nodiscard]] float SlopeCheckDistance  () const { return slopeCheckDistance_;   }
        /** 魔法が当たったときに撃ち手の画面に出すダメージ表記。未設定なら nullptr */
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> DealDamageTextBillBoardPrefab() const { return dealDamageTextBillBoardPrefab_.get(); }

    private:
        [[serialize(1)]] FIELD(MagicSpellData) basicSpell_;
        [[serialize(1)]] std::array<FIELD(MagicSpellData), LOADOUT_SLOT_COUNT> loadout_;
        [[serialize(0)]] float groundCheckRadius_   = 40.0f;
        [[serialize(0)]] float groundCheckUpOffset_ = 3.0f;
        [[serialize(0)]] float groundCheckDistance_ = 8.3f;
        [[serialize(2)]] float maxWalkableSlope_deg_ = 45.0f;
        [[serialize(2)]] float slopeCheckRadius_     = 3.5f;
        [[serialize(2)]] float slopeCheckUpOffset_   = 0.0f;
        [[serialize(2)]] float slopeCheckDistance_   = 2.5f;
        [[serialize(3)]] FIELD(PrefabGameObjectFile) dealDamageTextBillBoardPrefab_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(basicSpell_));
            for (size_t i = 0; i < loadout_.size(); ++i)
                archive(cereal::make_nvp("loadout_" + std::to_string(i), loadout_[i]));
            archive(CEREAL_NVP(groundCheckRadius_));
            archive(CEREAL_NVP(groundCheckUpOffset_));
            archive(CEREAL_NVP(groundCheckDistance_));
            archive(CEREAL_NVP(maxWalkableSlope_deg_));
            archive(CEREAL_NVP(slopeCheckRadius_));
            archive(CEREAL_NVP(slopeCheckUpOffset_));
            archive(CEREAL_NVP(slopeCheckDistance_));
            archive(CEREAL_NVP(dealDamageTextBillBoardPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version == 0)
            {
                // v0 は魔法弾1種だけの持ち方。値は魔法弾の .magicSpell へ移したので読み捨てる
                FIELD(PrefabGameObjectFile) magicBoltPrefab_;
                float magicBoltSpeed_         = 0.0f;
                float castFireTime_secs_      = 0.0f;
                float castTotalDuration_secs_ = 0.0f;
                archive(CEREAL_NVP(magicBoltPrefab_));
                archive(CEREAL_NVP(magicBoltSpeed_));
                archive(CEREAL_NVP(castFireTime_secs_));
                archive(CEREAL_NVP(castTotalDuration_secs_));
            }
            if (version >= 1)
            {
                archive(CEREAL_NVP(basicSpell_));
                for (size_t i = 0; i < loadout_.size(); ++i)
                    archive(cereal::make_nvp("loadout_" + std::to_string(i), loadout_[i]));
            }
            archive(CEREAL_NVP(groundCheckRadius_));
            archive(CEREAL_NVP(groundCheckUpOffset_));
            archive(CEREAL_NVP(groundCheckDistance_));
            if (version >= 2)
            {
                archive(CEREAL_NVP(maxWalkableSlope_deg_));
                archive(CEREAL_NVP(slopeCheckRadius_));
                archive(CEREAL_NVP(slopeCheckUpOffset_));
                archive(CEREAL_NVP(slopeCheckDistance_));
            }
            if (version >= 3) archive(CEREAL_NVP(dealDamageTextBillBoardPrefab_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(MagicCasterAvatarResource, MAGIC_CASTER_RESOURCE_EXTENSION_LABEL, "Player::MagicCaster")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::MagicCasterAvatarResource, 3);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::MagicCasterAvatarResource);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::MagicCasterAvatarResource);
#pragma endregion

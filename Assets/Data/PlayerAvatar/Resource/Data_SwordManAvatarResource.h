#pragma once
#include <string>
#include <vector>

#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Item/Data_ItemStack.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto SWORD_MAN_RESOURCE_EXTENSION_LABEL = ".swordManResource";

    class SwordManAvatarResource final : public ScriptableObject
    {
    public:
        explicit SwordManAvatarResource(const std::string& contentPath = "");
        [[nodiscard]] PrefabGameObjectFile& NormalAttackParticlePrefab() const { return *normalAttackParticlePrefab_.get(); }
        [[nodiscard]] PrefabGameObjectFile& DealDamageTextBillBoardPrefab() const { return *dealDamageTextBillBoardPrefab_.get(); }
        /** 攻撃が空振りしたときの風切り音。ため攻撃と、専用の音がないダッシュ/ジャンプ攻撃・段ぶんの音がないコンボで鳴る */
        [[nodiscard]] SoundFile& AttackWhiffSound() const { return *attackWhiffSound_.get(); }
        /** 攻撃が敵に当たったときの打撃音。ため攻撃と、専用の音がないダッシュ/ジャンプ攻撃・段ぶんの音がないコンボで鳴る */
        [[nodiscard]] SoundFile& AttackHitSound  () const { return *attackHitSound_  .get(); }
        [[nodiscard]] bool HasAttackHitSound() const { return static_cast<bool>(attackHitSound_); }
        /** 通常攻撃コンボの段ごとの空振り音。段が進むほど大きく振る想定で、足りない段は attackWhiffSound_ を鳴らす */
        [[nodiscard]] const std::vector<FIELD(SoundFile)>& ComboNormalAttackWhiffSounds() const { return comboNormalAttackWhiffSounds_; }
        /** 通常攻撃コンボの段ごとの打撃音。段が進むほど重く長い音を入れる想定で、足りない段は attackHitSound_ を鳴らす */
        [[nodiscard]] const std::vector<FIELD(SoundFile)>& ComboNormalAttackHitSounds() const { return comboNormalAttackHitSounds_; }
        /** ダッシュ攻撃の空振り/ヒット音。未設定なら attackWhiffSound_ / attackHitSound_ を鳴らす */
        [[nodiscard]] const FIELD(SoundFile)& DashAttackWhiffSound() const { return dashAttackWhiffSound_; }
        [[nodiscard]] const FIELD(SoundFile)& DashAttackHitSound  () const { return dashAttackHitSound_;   }
        /** ジャンプ攻撃の着地の叩きつけの空振り/ヒット音。未設定なら attackWhiffSound_ / attackHitSound_ を鳴らす */
        [[nodiscard]] const FIELD(SoundFile)& JumpAttackWhiffSound() const { return jumpAttackWhiffSound_; }
        [[nodiscard]] const FIELD(SoundFile)& JumpAttackHitSound  () const { return jumpAttackHitSound_;   }
        /** ジャンプ攻撃の振りかぶりを終えて急降下を始めた瞬間に1回鳴らす */
        [[nodiscard]] SoundFile& JumpAttackPlungeSound() const { return *jumpAttackPlungeSound_.get(); }
        [[nodiscard]] bool HasJumpAttackPlungeSound() const { return static_cast<bool>(jumpAttackPlungeSound_); }
        /** ステージ開始時に持っているアイテム。ポーチはセーブに乗せず毎回ここから作り直す */
        [[nodiscard]] const std::vector<ItemStack>& InitialItems() const { return initialItems_; }
        [[nodiscard]] SoundFile& AvoidRollingSound    () const { return *avoidRollingSound_    .get(); }
        [[nodiscard]] SoundFile& JustAvoidRollingSound() const { return *justAvoidRollingSound_.get(); }
        [[nodiscard]] SoundFile& JumpSound() const { return *jumpSound_.get(); }

        /** 着地した瞬間に足元へ1回生成する土煙。落ちてきた速さで大きさを変える */
        [[nodiscard]] PrefabGameObjectFile& LandingParticlePrefab() const { return *landingParticlePrefab_.get(); }
        [[nodiscard]] bool HasLandingParticlePrefab() const { return static_cast<bool>(landingParticlePrefab_); }
        /** この落下速度に届かない着地では土煙を出さない。段差を降りたくらいで出さないための下限 */
        [[nodiscard]] float LandingParticleMinFallSpeed() const { return landingParticleMinFallSpeed_; }
        /** 着地の土煙が最大の大きさになる落下速度 */
        [[nodiscard]] float LandingParticleMaxFallSpeed() const { return landingParticleMaxFallSpeed_; }
        /** 最低落下速度で着地したときの、プレハブのスケールに掛ける倍率 */
        [[nodiscard]] float LandingParticleMinScale() const { return landingParticleMinScale_; }
        /** 最大落下速度で着地したときの、プレハブのスケールに掛ける倍率 */
        [[nodiscard]] float LandingParticleMaxScale() const { return landingParticleMaxScale_; }

        [[nodiscard]] PrefabGameObjectFile& FootstepParticlePrefab() const { return *footstepParticlePrefab_.get(); }
        [[nodiscard]] bool HasFootstepParticlePrefab() const { return static_cast<bool>(footstepParticlePrefab_); }
        /** 接地を見る足ボーンの名前。左右それぞれ1本ずつ入れる想定 */
        [[nodiscard]] const std::vector<std::string>& FootstepBoneNames() const { return footstepBoneNames_; }
        /** 足ボーンが足元(FeatStep)からこの高さまで降りてきたら接地扱いにする。一度この高さを超えるまで次は鳴らない */
        [[nodiscard]] float FootstepContactHeight() const { return footstepContactHeight_; }
        /** 歩き系クリップの足音候補。鳴らすときに配列からランダムで1つ選択する */
        [[nodiscard]] const std::vector<FIELD(SoundFile)>& WalkFootstepSounds() const { return walkFootstepSounds_; }
        /** 走り系クリップの足音候補。鳴らすときに配列からランダムで1つ選択する */
        [[nodiscard]] const std::vector<FIELD(SoundFile)>& RunFootstepSounds()  const { return runFootstepSounds_; }
        /** ため攻撃が最大溜めに達した瞬間に1回鳴らす */
        [[nodiscard]] SoundFile& ChargeCompleteSound() const { return *chargeCompleteSound_.get(); }
        [[nodiscard]] bool HasChargeCompleteSound() const { return static_cast<bool>(chargeCompleteSound_); }
        /** ため攻撃が最大溜めに達した瞬間にプレイヤー位置へ1回生成する */
        [[nodiscard]] PrefabGameObjectFile& ChargeCompleteParticlePrefab() const { return *chargeCompleteParticlePrefab_.get(); }
        [[nodiscard]] bool HasChargeCompleteParticlePrefab() const { return static_cast<bool>(chargeCompleteParticlePrefab_); }
        /** 最大溜めのまま保持している間だけ出し続けるオーラ（ステート側で破棄する） */
        [[nodiscard]] PrefabGameObjectFile& ChargeHoldParticlePrefab() const { return *chargeHoldParticlePrefab_.get(); }
        [[nodiscard]] bool HasChargeHoldParticlePrefab() const { return static_cast<bool>(chargeHoldParticlePrefab_); }
        /** ため攻撃の叩きつけ発生時に、衝突点の地面へ1回生成する */
        [[nodiscard]] PrefabGameObjectFile& ChargeImpactParticlePrefab() const { return *chargeImpactParticlePrefab_.get(); }
        [[nodiscard]] bool HasChargeImpactParticlePrefab() const { return static_cast<bool>(chargeImpactParticlePrefab_); }
        /** 攻撃が壁に阻まれたときに、Raycastの衝突点へ1回生成する火花 */
        [[nodiscard]] PrefabGameObjectFile& AttackBlockedParticlePrefab() const { return *attackBlockedParticlePrefab_.get(); }
        [[nodiscard]] bool HasAttackBlockedParticlePrefab() const { return static_cast<bool>(attackBlockedParticlePrefab_); }
        /** 攻撃が壁に阻まれたときの金属音の候補。鳴らすときに配列からランダムで1つ選択する */
        [[nodiscard]] const std::vector<FIELD(SoundFile)>& AttackBlockedSounds() const { return attackBlockedSounds_; }
        /** 溜め中の持続カメラ揺れ。溜め開始時が Min、最大溜め直前が Max */
        [[nodiscard]] float ChargingShakeIntensityMin() const { return chargingShakeIntensityMin_; }
        [[nodiscard]] float ChargingShakeIntensityMax() const { return chargingShakeIntensityMax_; }
        /** 最大溜めのまま保持している間の持続カメラ揺れ */
        [[nodiscard]] float ChargedHoldShakeIntensity() const { return chargedHoldShakeIntensity_; }
        /** 最大溜めに達した瞬間に1回だけ入れるカメラ揺れ */
        [[nodiscard]] float ChargeCompleteShakeIntensity    () const { return chargeCompleteShakeIntensity_; }
        [[nodiscard]] float ChargeCompleteShakeDuration_secs() const { return chargeCompleteShakeDuration_secs_; }
        /** 接地判定SphereCastの半径。カプセルの半径より小さくし、横の壁に触れているだけで接地扱いにならないようにする */
        [[nodiscard]] float GroundCheckRadius  () const { return groundCheckRadius_;   }
        /** 接地判定SphereCastの開始時、球の下端を足元からどれだけ上に置くか */
        [[nodiscard]] float GroundCheckUpOffset() const { return groundCheckUpOffset_; }
        /** 接地判定SphereCastの下方向への探索距離 */
        [[nodiscard]] float GroundCheckDistance() const { return groundCheckDistance_; }
        /** 歩き・走り・踏み込みで登れる斜面の最大角度。これより急な面へ向かう速度は消す */
        [[nodiscard]] float MaxWalkableSlope_deg() const { return maxWalkableSlope_deg_; }
        /** 斜面判定SphereCastの半径。カプセルの半径より少し小さくする */
        [[nodiscard]] float SlopeCheckRadius    () const { return slopeCheckRadius_;     }
        /** 斜面判定SphereCastの球の下端を足元からどれだけ上に置くか。これより低い段差は判定に掛からない */
        [[nodiscard]] float SlopeCheckUpOffset  () const { return slopeCheckUpOffset_;   }
        /** 斜面判定SphereCastの進行方向への探索距離 */
        [[nodiscard]] float SlopeCheckDistance  () const { return slopeCheckDistance_;   }
        [[nodiscard]] float WalkAccelerationTime_secs() const { return walkAccelerationTime_secs_; }
        [[nodiscard]] float RunAccelerationTime_secs () const { return runAccelerationTime_secs_;  }
        [[nodiscard]] float WalkDecelerationTime_secs() const { return walkDecelerationTime_secs_; }
        [[nodiscard]] float RunDecelerationTime_secs () const { return runDecelerationTime_secs_;  }

    private:
        [[serialize(0)]] FIELD(PrefabGameObjectFile) normalAttackParticlePrefab_;
        [[serialize(2)]] FIELD(PrefabGameObjectFile) dealDamageTextBillBoardPrefab_;
        [[serialize(0)]] FIELD(SoundFile) attackWhiffSound_;
        [[serialize(0)]] FIELD(SoundFile) avoidRollingSound_;
        [[serialize(0)]] FIELD(SoundFile) justAvoidRollingSound_;
        [[serialize(0)]] FIELD(SoundFile) jumpSound_;
        [[serialize(3)]] FIELD(PrefabGameObjectFile) footstepParticlePrefab_;
        [[serialize(4)]] std::vector<FIELD(SoundFile)> walkFootstepSounds_;
        [[serialize(4)]] std::vector<FIELD(SoundFile)> runFootstepSounds_;
        [[serialize(5)]] FIELD(SoundFile)            chargeCompleteSound_;
        [[serialize(5)]] FIELD(PrefabGameObjectFile) chargeCompleteParticlePrefab_;
        [[serialize(6)]] float                       groundCheckRadius_   = 40.0f;
        [[serialize(6)]] float                       groundCheckUpOffset_ = 3.0f;
        [[serialize(6)]] float                       groundCheckDistance_ = 8.3f;
        [[serialize(7)]] FIELD(PrefabGameObjectFile) chargeHoldParticlePrefab_;
        [[serialize(7)]] FIELD(PrefabGameObjectFile) chargeImpactParticlePrefab_;
        [[serialize(8)]] float                       walkAccelerationTime_secs_ = 0.25f; 
        [[serialize(8)]] float                       runAccelerationTime_secs_  = 0.15f;
        [[serialize(8)]] float                       walkDecelerationTime_secs_ = 0.15f;
        [[serialize(8)]] float                       runDecelerationTime_secs_  = 0.15f;
        [[serialize(9)]] float                       chargingShakeIntensityMin_        = 0.25f;
        [[serialize(9)]] float                       chargingShakeIntensityMax_        = 0.5f;
        [[serialize(9)]] float                       chargedHoldShakeIntensity_        = 0.6f;
        [[serialize(9)]] float                       chargeCompleteShakeIntensity_     = 0.5f;
        [[serialize(9)]] float                       chargeCompleteShakeDuration_secs_ = 0.25f;
        [[serialize(10)]] FIELD(PrefabGameObjectFile) attackBlockedParticlePrefab_;
        [[serialize(12)]] std::vector<FIELD(SoundFile)> attackBlockedSounds_;
        [[serialize(13)]] std::vector<std::string>    footstepBoneNames_;
        [[serialize(13)]] float                       footstepContactHeight_ = 5.0f;
        [[serialize(14)]] std::vector<FIELD(SoundFile)> comboNormalAttackHitSounds_;
        [[serialize(15)]] FIELD(SoundFile)              attackHitSound_;
        [[serialize(15)]] std::vector<FIELD(SoundFile)> comboNormalAttackWhiffSounds_;
        [[serialize(16)]] std::vector<ItemStack>        initialItems_;
        [[serialize(17)]] FIELD(PrefabGameObjectFile)   landingParticlePrefab_;
        [[serialize(17)]] float                         landingParticleMinFallSpeed_ = 45.0f;
        [[serialize(17)]] float                         landingParticleMaxFallSpeed_ = 260.0f;
        [[serialize(17)]] float                         landingParticleMinScale_     = 0.7f;
        [[serialize(17)]] float                         landingParticleMaxScale_     = 1.6f;
        [[serialize(21)]] float                         maxWalkableSlope_deg_        = 45.0f;
        [[serialize(21)]] float                         slopeCheckRadius_            = 3.5f;
        [[serialize(21)]] float                         slopeCheckUpOffset_          = 0.0f;
        [[serialize(21)]] float                         slopeCheckDistance_          = 2.5f;
        [[serialize(22)]] FIELD(SoundFile)              dashAttackWhiffSound_;
        [[serialize(22)]] FIELD(SoundFile)              dashAttackHitSound_;
        [[serialize(22)]] FIELD(SoundFile)              jumpAttackWhiffSound_;
        [[serialize(22)]] FIELD(SoundFile)              jumpAttackHitSound_;
        [[serialize(22)]] FIELD(SoundFile)              jumpAttackPlungeSound_;

        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(normalAttackParticlePrefab_));
            archive(CEREAL_NVP(dealDamageTextBillBoardPrefab_));
            archive(CEREAL_NVP(attackWhiffSound_));
            archive(CEREAL_NVP(avoidRollingSound_));
            archive(CEREAL_NVP(justAvoidRollingSound_));
            archive(CEREAL_NVP(jumpSound_));
            archive(CEREAL_NVP(footstepParticlePrefab_));

            archive(cereal::make_nvp("walkFootstepSoundCount", static_cast<std::uint32_t>(walkFootstepSounds_.size())));
            for (size_t i = 0; i < walkFootstepSounds_.size(); ++i)
                archive(cereal::make_nvp("walkFootstepSound_" + std::to_string(i), walkFootstepSounds_[i]));

            archive(cereal::make_nvp("runFootstepSoundCount", static_cast<std::uint32_t>(runFootstepSounds_.size())));
            for (size_t i = 0; i < runFootstepSounds_.size(); ++i)
                archive(cereal::make_nvp("runFootstepSound_" + std::to_string(i), runFootstepSounds_[i]));

            archive(CEREAL_NVP(chargeCompleteSound_));
            archive(CEREAL_NVP(chargeCompleteParticlePrefab_));
            archive(CEREAL_NVP(groundCheckRadius_));
            archive(CEREAL_NVP(groundCheckUpOffset_));
            archive(CEREAL_NVP(groundCheckDistance_));
            archive(CEREAL_NVP(chargeHoldParticlePrefab_));
            archive(CEREAL_NVP(chargeImpactParticlePrefab_));
            archive(CEREAL_NVP(walkAccelerationTime_secs_));
            archive(CEREAL_NVP(runAccelerationTime_secs_));
            archive(CEREAL_NVP(walkDecelerationTime_secs_));
            archive(CEREAL_NVP(runDecelerationTime_secs_));
            archive(CEREAL_NVP(chargingShakeIntensityMin_));
            archive(CEREAL_NVP(chargingShakeIntensityMax_));
            archive(CEREAL_NVP(chargedHoldShakeIntensity_));
            archive(CEREAL_NVP(chargeCompleteShakeIntensity_));
            archive(CEREAL_NVP(chargeCompleteShakeDuration_secs_));
            archive(CEREAL_NVP(attackBlockedParticlePrefab_));

            archive(cereal::make_nvp("attackBlockedSoundCount", static_cast<std::uint32_t>(attackBlockedSounds_.size())));
            for (size_t i = 0; i < attackBlockedSounds_.size(); ++i)
                archive(cereal::make_nvp("attackBlockedSound_" + std::to_string(i), attackBlockedSounds_[i]));

            archive(CEREAL_NVP(footstepBoneNames_));
            archive(CEREAL_NVP(footstepContactHeight_));

            archive(cereal::make_nvp("comboNormalAttackHitSoundCount", static_cast<std::uint32_t>(comboNormalAttackHitSounds_.size())));
            for (size_t i = 0; i < comboNormalAttackHitSounds_.size(); ++i)
                archive(cereal::make_nvp("comboNormalAttackHitSound_" + std::to_string(i), comboNormalAttackHitSounds_[i]));

            archive(CEREAL_NVP(attackHitSound_));

            archive(cereal::make_nvp("comboNormalAttackWhiffSoundCount", static_cast<std::uint32_t>(comboNormalAttackWhiffSounds_.size())));
            for (size_t i = 0; i < comboNormalAttackWhiffSounds_.size(); ++i)
                archive(cereal::make_nvp("comboNormalAttackWhiffSound_" + std::to_string(i), comboNormalAttackWhiffSounds_[i]));

            archive(cereal::make_nvp("initialItemCount", static_cast<std::uint32_t>(initialItems_.size())));
            for (size_t i = 0; i < initialItems_.size(); ++i)
                archive(cereal::make_nvp("initialItem_" + std::to_string(i), initialItems_[i]));

            archive(CEREAL_NVP(landingParticlePrefab_));
            archive(CEREAL_NVP(landingParticleMinFallSpeed_));
            archive(CEREAL_NVP(landingParticleMaxFallSpeed_));
            archive(CEREAL_NVP(landingParticleMinScale_));
            archive(CEREAL_NVP(landingParticleMaxScale_));
            archive(CEREAL_NVP(maxWalkableSlope_deg_));
            archive(CEREAL_NVP(slopeCheckRadius_));
            archive(CEREAL_NVP(slopeCheckUpOffset_));
            archive(CEREAL_NVP(slopeCheckDistance_));
            archive(CEREAL_NVP(dashAttackWhiffSound_));
            archive(CEREAL_NVP(dashAttackHitSound_));
            archive(CEREAL_NVP(jumpAttackWhiffSound_));
            archive(CEREAL_NVP(jumpAttackHitSound_));
            archive(CEREAL_NVP(jumpAttackPlungeSound_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(normalAttackParticlePrefab_));
            if (version >= 2) archive(CEREAL_NVP(dealDamageTextBillBoardPrefab_));
            // v15で当たり外れの鳴り分けを入れ、それまでの normalAttackSound_ は空振り側の音になった
            if (version >= 15)     archive(CEREAL_NVP(attackWhiffSound_));
            else if (version >= 0) archive(cereal::make_nvp("normalAttackSound_", attackWhiffSound_));
            if (version >= 0) archive(CEREAL_NVP(avoidRollingSound_));
            if (version >= 0) archive(CEREAL_NVP(justAvoidRollingSound_));
            if (version >= 1) archive(CEREAL_NVP(jumpSound_));
            if (version >= 3) archive(CEREAL_NVP(footstepParticlePrefab_));
            if (version >= 3 && version < 13)
            {
                // v13で接地タイミングの手入力をやめ、足ボーンの高さで判定するようにした
                std::vector<float> legacyFootstepContactPhases;
                archive(cereal::make_nvp("walkFootstepContactPhases_", legacyFootstepContactPhases));
                archive(cereal::make_nvp("runFootstepContactPhases_", legacyFootstepContactPhases));
            }
            if (version >= 4)
            {
                std::uint32_t walkFootstepSoundCount = 0;
                archive(cereal::make_nvp("walkFootstepSoundCount", walkFootstepSoundCount));
                walkFootstepSounds_.resize(walkFootstepSoundCount);
                for (size_t i = 0; i < walkFootstepSoundCount; ++i)
                    archive(cereal::make_nvp("walkFootstepSound_" + std::to_string(i), walkFootstepSounds_[i]));

                std::uint32_t runFootstepSoundCount = 0;
                archive(cereal::make_nvp("runFootstepSoundCount", runFootstepSoundCount));
                runFootstepSounds_.resize(runFootstepSoundCount);
                for (size_t i = 0; i < runFootstepSoundCount; ++i)
                    archive(cereal::make_nvp("runFootstepSound_" + std::to_string(i), runFootstepSounds_[i]));
            }
            if (version >= 5) archive(CEREAL_NVP(chargeCompleteSound_));
            if (version >= 5) archive(CEREAL_NVP(chargeCompleteParticlePrefab_));
            if (version >= 6) archive(CEREAL_NVP(groundCheckRadius_));
            if (version >= 6) archive(CEREAL_NVP(groundCheckUpOffset_));
            if (version >= 6) archive(CEREAL_NVP(groundCheckDistance_));
            if (version >= 7) archive(CEREAL_NVP(chargeHoldParticlePrefab_));
            if (version >= 7) archive(CEREAL_NVP(chargeImpactParticlePrefab_));
            if (version >= 8) archive(CEREAL_NVP(walkAccelerationTime_secs_));
            if (version >= 8) archive(CEREAL_NVP(runAccelerationTime_secs_));
            if (version >= 8) archive(CEREAL_NVP(walkDecelerationTime_secs_));
            if (version >= 8) archive(CEREAL_NVP(runDecelerationTime_secs_));
            if (version >= 9) archive(CEREAL_NVP(chargingShakeIntensityMin_));
            if (version >= 9) archive(CEREAL_NVP(chargingShakeIntensityMax_));
            if (version >= 9) archive(CEREAL_NVP(chargedHoldShakeIntensity_));
            if (version >= 9) archive(CEREAL_NVP(chargeCompleteShakeIntensity_));
            if (version >= 9) archive(CEREAL_NVP(chargeCompleteShakeDuration_secs_));
            if (version >= 10) archive(CEREAL_NVP(attackBlockedParticlePrefab_));
            if (version == 11)
            {
                // v11は単数フィールドだった。候補1つの配列として読み替える
                attackBlockedSounds_.resize(1);
                archive(cereal::make_nvp("attackBlockedSound_", attackBlockedSounds_[0]));
            }
            else if (version >= 12)
            {
                std::uint32_t attackBlockedSoundCount = 0;
                archive(cereal::make_nvp("attackBlockedSoundCount", attackBlockedSoundCount));
                attackBlockedSounds_.resize(attackBlockedSoundCount);
                for (size_t i = 0; i < attackBlockedSoundCount; ++i)
                    archive(cereal::make_nvp("attackBlockedSound_" + std::to_string(i), attackBlockedSounds_[i]));
            }
            if (version >= 13) archive(CEREAL_NVP(footstepBoneNames_));
            if (version >= 13) archive(CEREAL_NVP(footstepContactHeight_));
            if (version == 14)
            {
                // v14は当たり外れで鳴り分けていなかった。当たったときの音として読む
                std::uint32_t comboNormalAttackSoundCount = 0;
                archive(cereal::make_nvp("comboNormalAttackSoundCount", comboNormalAttackSoundCount));
                comboNormalAttackHitSounds_.resize(comboNormalAttackSoundCount);
                for (size_t i = 0; i < comboNormalAttackSoundCount; ++i)
                    archive(cereal::make_nvp("comboNormalAttackSound_" + std::to_string(i), comboNormalAttackHitSounds_[i]));
            }
            else if (version >= 15)
            {
                std::uint32_t comboNormalAttackHitSoundCount = 0;
                archive(cereal::make_nvp("comboNormalAttackHitSoundCount", comboNormalAttackHitSoundCount));
                comboNormalAttackHitSounds_.resize(comboNormalAttackHitSoundCount);
                for (size_t i = 0; i < comboNormalAttackHitSoundCount; ++i)
                    archive(cereal::make_nvp("comboNormalAttackHitSound_" + std::to_string(i), comboNormalAttackHitSounds_[i]));

                archive(CEREAL_NVP(attackHitSound_));

                std::uint32_t comboNormalAttackWhiffSoundCount = 0;
                archive(cereal::make_nvp("comboNormalAttackWhiffSoundCount", comboNormalAttackWhiffSoundCount));
                comboNormalAttackWhiffSounds_.resize(comboNormalAttackWhiffSoundCount);
                for (size_t i = 0; i < comboNormalAttackWhiffSoundCount; ++i)
                    archive(cereal::make_nvp("comboNormalAttackWhiffSound_" + std::to_string(i), comboNormalAttackWhiffSounds_[i]));
            }

            if (version >= 16)
            {
                std::uint32_t initialItemCount = 0;
                archive(cereal::make_nvp("initialItemCount", initialItemCount));
                initialItems_.resize(initialItemCount);
                for (size_t i = 0; i < initialItemCount; ++i)
                    archive(cereal::make_nvp("initialItem_" + std::to_string(i), initialItems_[i]));
            }

            if (version >= 17)
            {
                if (version < 20)
                {
                    // v20 でジャンプの踏み切りの土煙をやめたので読み捨てる
                    FIELD(PrefabGameObjectFile) jumpParticlePrefab_;
                    archive(CEREAL_NVP(jumpParticlePrefab_));
                }
                archive(CEREAL_NVP(landingParticlePrefab_));
                archive(CEREAL_NVP(landingParticleMinFallSpeed_));
                archive(CEREAL_NVP(landingParticleMaxFallSpeed_));
                archive(CEREAL_NVP(landingParticleMinScale_));
                archive(CEREAL_NVP(landingParticleMaxScale_));
            }

            if (version == 18)
            {
                // v18 はポータルから地中を通ってせり上がる登場の尺と深さを持っていた。今は歩いて出てくるので読み捨てる
                float warpInRise_secs_ = 0.0f;
                float warpInSinkDepth_ = 0.0f;
                archive(CEREAL_NVP(warpInRise_secs_));
                archive(CEREAL_NVP(warpInSinkDepth_));
            }

            if (version >= 21)
            {
                archive(CEREAL_NVP(maxWalkableSlope_deg_));
                archive(CEREAL_NVP(slopeCheckRadius_));
                archive(CEREAL_NVP(slopeCheckUpOffset_));
                archive(CEREAL_NVP(slopeCheckDistance_));
            }

            if (version >= 22)
            {
                archive(CEREAL_NVP(dashAttackWhiffSound_));
                archive(CEREAL_NVP(dashAttackHitSound_));
                archive(CEREAL_NVP(jumpAttackWhiffSound_));
                archive(CEREAL_NVP(jumpAttackHitSound_));
                archive(CEREAL_NVP(jumpAttackPlungeSound_));
            }
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SwordManAvatarResource, 22);
#pragma endregion

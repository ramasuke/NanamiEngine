#pragma once
#include <cstdint>

#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar
{
    /** @brief 攻撃がヒットした瞬間の演出パラメータ(カメラシェイク・パーティクル拡大率・被弾モデルの揺れ) */
    struct HitFeelParam final
    {
        explicit HitFeelParam(
            float shakeIntensity = 0.0f,
            float shakeDuration_secs = 0.0f,
            float particleScale = 0.0f,
            float targetShakeAmplitude = 0.0f,
            float targetShakeDuration_secs = 0.0f);

        /** @brief カメラシェイクの強度 */
        [[nodiscard]] float ShakeIntensity      () const { return shakeIntensity_;       }
        /** @brief カメラシェイクの長さ[秒] */
        [[nodiscard]] float ShakeDuration_secs  () const { return shakeDuration_secs_;   }
        /** @brief ヒットパーティクルの拡大率(1.0が等倍) */
        [[nodiscard]] float ParticleScale       () const { return particleScale_;        }
        /** @brief 被弾した相手モデルの揺れ幅[ワールド単位]。攻撃した本人の画面だけで揺らすローカル演出 */
        [[nodiscard]] float TargetShakeAmplitude    () const { return targetShakeAmplitude_;     }
        /** @brief 被弾した相手モデルの揺れの長さ[秒] */
        [[nodiscard]] float TargetShakeDuration_secs() const { return targetShakeDuration_secs_; }

    private:
        [[serialize(0)]] float shakeIntensity_;
        [[serialize(0)]] float shakeDuration_secs_;
        [[serialize(0)]] float particleScale_;
        [[serialize(1)]] float targetShakeAmplitude_;
        [[serialize(1)]] float targetShakeDuration_secs_;

#pragma region Serialization Function
    public:
        void OnDrawGui();
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(shakeIntensity_);
            archive(shakeDuration_secs_);
            archive(particleScale_);
            archive(targetShakeAmplitude_);
            archive(targetShakeDuration_secs_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version < 2)
            {
                // 削除済みのヒットストップ値(長さ・timeScale)を読み捨てる
                float legacyHitStopDuration_secs = 0.0f;
                float legacyHitStopTimeScale = 0.0f;
                archive(legacyHitStopDuration_secs);
                archive(legacyHitStopTimeScale);
            }
            archive(shakeIntensity_);
            archive(shakeDuration_secs_);
            archive(particleScale_);
            if (version >= 1) archive(targetShakeAmplitude_);
            if (version >= 1) archive(targetShakeDuration_secs_);
        }
#pragma endregion
    };

    inline HitFeelParam::HitFeelParam(
        const float shakeIntensity,
        const float shakeDuration_secs,
        const float particleScale,
        const float targetShakeAmplitude,
        const float targetShakeDuration_secs)
            : shakeIntensity_(shakeIntensity)
            , shakeDuration_secs_(shakeDuration_secs)
            , particleScale_(particleScale)
            , targetShakeAmplitude_(targetShakeAmplitude)
            , targetShakeDuration_secs_(targetShakeDuration_secs)
    {
    }
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::HitFeelParam, 2)

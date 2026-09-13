#pragma once
#include <cstdint>

#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar
{
    /** @brief 攻撃がヒットした瞬間の演出パラメータ(ヒットストップ・カメラシェイク・パーティクル拡大率・被弾モデルの揺れ) */
    struct HitFeelParam final
    {
        explicit HitFeelParam(
            float hitStopDuration_secs = 0.0f,
            float hitStopTimeScale = 0.0f,
            float shakeIntensity = 0.0f,
            float shakeDuration_secs = 0.0f,
            float particleScale = 0.0f,
            float targetShakeAmplitude = 0.0f,
            float targetShakeDuration_secs = 0.0f);

        /** @brief ヒットストップの長さ[秒]。自機Animatorのみに適用するローカル演出 */
        [[nodiscard]] float HitStopDuration_secs() const { return hitStopDuration_secs_; }
        /** @brief ヒットストップ中のAnimator timeScale(0に近いほど強く止まる) */
        [[nodiscard]] float HitStopTimeScale    () const { return hitStopTimeScale_;     }
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
        [[serialize(0)]] float hitStopDuration_secs_;
        [[serialize(0)]] float hitStopTimeScale_;
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
            archive(hitStopDuration_secs_);
            archive(hitStopTimeScale_);
            archive(shakeIntensity_);
            archive(shakeDuration_secs_);
            archive(particleScale_);
            archive(targetShakeAmplitude_);
            archive(targetShakeDuration_secs_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(hitStopDuration_secs_);
            archive(hitStopTimeScale_);
            archive(shakeIntensity_);
            archive(shakeDuration_secs_);
            archive(particleScale_);
            if (version >= 1) archive(targetShakeAmplitude_);
            if (version >= 1) archive(targetShakeDuration_secs_);
        }
#pragma endregion
    };

    inline HitFeelParam::HitFeelParam(
        const float hitStopDuration_secs,
        const float hitStopTimeScale,
        const float shakeIntensity,
        const float shakeDuration_secs,
        const float particleScale,
        const float targetShakeAmplitude,
        const float targetShakeDuration_secs)
            : hitStopDuration_secs_(hitStopDuration_secs)
            , hitStopTimeScale_(hitStopTimeScale)
            , shakeIntensity_(shakeIntensity)
            , shakeDuration_secs_(shakeDuration_secs)
            , particleScale_(particleScale)
            , targetShakeAmplitude_(targetShakeAmplitude)
            , targetShakeDuration_secs_(targetShakeDuration_secs)
    {
    }
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::HitFeelParam, 1)

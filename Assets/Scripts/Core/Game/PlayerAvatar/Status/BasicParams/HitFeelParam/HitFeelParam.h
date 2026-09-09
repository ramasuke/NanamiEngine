#pragma once
#include <cstdint>

#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar
{
    /** @brief 攻撃がヒットした瞬間の演出パラメータ(ヒットストップ・カメラシェイク・パーティクル拡大率) */
    struct HitFeelParam final
    {
        explicit HitFeelParam(
            float hitStopDuration_secs,
            float hitStopTimeScale,
            float shakeIntensity,
            float shakeDuration_secs,
            float particleScale);

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

    private:
        [[serialize(0)]] float hitStopDuration_secs_;
        [[serialize(0)]] float hitStopTimeScale_;
        [[serialize(0)]] float shakeIntensity_;
        [[serialize(0)]] float shakeDuration_secs_;
        [[serialize(0)]] float particleScale_;

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
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(hitStopDuration_secs_);
            archive(hitStopTimeScale_);
            archive(shakeIntensity_);
            archive(shakeDuration_secs_);
            archive(particleScale_);
        }
#pragma endregion
    };

    inline HitFeelParam::HitFeelParam(
        const float hitStopDuration_secs,
        const float hitStopTimeScale,
        const float shakeIntensity,
        const float shakeDuration_secs,
        const float particleScale)
            : hitStopDuration_secs_(hitStopDuration_secs)
            , hitStopTimeScale_(hitStopTimeScale)
            , shakeIntensity_(shakeIntensity)
            , shakeDuration_secs_(shakeDuration_secs)
            , particleScale_(particleScale)
    {
    }
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::HitFeelParam, 0)

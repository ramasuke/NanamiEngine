#pragma once
#include <random>
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/Component/Skydome3D/SkyDome3D.h"
#include "../../../../Engine/Module/Component/Rotator/Rotator.h"
#include "../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../Engine/Module/Asset/Sound/SoundFile.h"

namespace GamePlay::Weather
{
    class WeatherService final : public Component::ComponentBase,
                                 public LifeCycleCallback::IAwakable,
                                 public LifeCycleCallback::IUpdatable
    {
    public:
        static WeatherService* Instance() { return instance_; }

        /** @brief 嵐の強さを blendSeconds かけて targetIntensity(0..1) へ寄せる */
        void SetStorm(float targetIntensity, float blendSeconds);
        /** @brief 落雷。閃光と、thunderDelay_secs_ 後の雷鳴 */
        void Lightning(float intensity, float durationSeconds);
        [[nodiscard]] float GetStormIntensity() const { return stormIntensity_; }
        /** @brief 既にその強さを目標にしているか。BTアクションが毎フレーム同じ指示を送らないための判定 */
        [[nodiscard]] bool HasStormTarget(const float targetIntensity) const
        {
            return std::clamp(targetIntensity, 0.0f, 1.0f) == stormTarget_;
        }

    private:
        void OnAwake  () override;
        void OnUpdate () override;
        void OnDestroy() override;

        void Flash(float intensity, float durationSeconds);
        void UpdateIntensity     (float deltaTime);
        void UpdateLightning     (float deltaTime);
        void UpdatePendingThunder(float deltaTime);
        void UpdateDistantThunder(float deltaTime);
        void ApplySky      () const;
        void ApplyFog      () const;
        void ApplyLight    () const;
        void ApplyShake    () const;
        void ApplyParticles();
        void RestoreClearWeather() const;
        [[nodiscard]] float LightningBrightness() const;

        static WeatherService* instance_;

        [[serialize(0)]] FIELD(Component::SkyDome3D)         skyDomeUpper_;
        [[serialize(0)]] FIELD(Component::SkyDome3D)         skyDomeLower_;
        [[serialize(0)]] FIELD(Component::Rotator)           upperRotator_;
        [[serialize(0)]] FIELD(Component::Rotator)           lowerRotator_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) flashRenderer_;
        [[serialize(0)]] FIELD(Component::ParticleSystem)    wispParticle_;
        [[serialize(0)]] FIELD(Component::ParticleSystem)    gustParticle_;
        [[serialize(0)]] FIELD(Asset::SoundFile)             thunderNearSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile)             thunderFarSound1_;
        [[serialize(0)]] FIELD(Asset::SoundFile)             thunderFarSound2_;

        [[serialize(0)]] glm::vec3 clearSkyTint_ = glm::vec3(1.00f, 1.00f, 1.00f);
        [[serialize(0)]] glm::vec3 stormSkyTint_ = glm::vec3(0.26f, 0.29f, 0.36f);
        [[serialize(0)]] float clearUpperRotateSpeedDeg_ =   0.35f;
        [[serialize(0)]] float stormUpperRotateSpeedDeg_ =   8.00f;
        [[serialize(0)]] float clearLowerRotateSpeedDeg_ =  -0.60f;
        [[serialize(0)]] float stormLowerRotateSpeedDeg_ = -14.00f;
        [[serialize(0)]] glm::vec3 stormFogColor_ = glm::vec3(0.27f, 0.29f, 0.33f);
        [[serialize(0)]] float clearFogStart_ = 1200.0f;
        [[serialize(0)]] float clearFogEnd_   = 5000.0f;
        [[serialize(0)]] float stormFogStart_ =  150.0f;
        [[serialize(0)]] float stormFogEnd_   =  900.0f;
        [[serialize(0)]] glm::vec3 stormLightColor_ = glm::vec3(0.45f, 0.48f, 0.58f);
        [[serialize(0)]] float maxSustainShake_       = 0.06f;
        [[serialize(0)]] float particlePlayThreshold_ = 0.25f;
        [[serialize(0)]] int   flashMaxBlendRate_     = 235;
        [[serialize(0)]] float lightningLightBoost_   = 1.6f;
        [[serialize(0)]] float thunderDelay_secs_     = 0.25f;
        // 1発目の鋭いピークが減衰しきる前に2発目を重ねると「近い雷」に見える
        [[serialize(0)]] float firstFlashDecay_secs_  = 0.05f;
        [[serialize(0)]] float secondFlashDelay_secs_ = 0.11f;
        [[serialize(0)]] float secondFlashDecay_secs_ = 0.09f;
        [[serialize(0)]] float secondFlashStrength_   = 0.65f;
        // 落雷で空そのものが白く抜ける量。1.0にすると白飛びしすぎる
        [[serialize(0)]] float skyFlashWeight_ = 0.7f;
        [[serialize(0)]] float distantFlashStrength_      = 0.28f;
        [[serialize(0)]] float distantFlashDuration_secs_ = 0.5f;
        [[serialize(0)]] float distantThunderThreshold_        = 0.5f;
        [[serialize(0)]] float distantThunderMinInterval_secs_ = 6.0f;
        [[serialize(0)]] float distantThunderMaxInterval_secs_ = 10.0f;

        float stormIntensity_   = 0.0f;
        float stormTarget_      = 0.0f;
        float stormBlendSpeed_  = 0.0f;
        float lightningStrength_ = 0.0f;
        float lightningElapsed_  = 0.0f;
        float lightningDuration_ = 0.0f;
        float pendingThunder_secs_ = -1.0f;
        float distantThunderTimer_secs_ = 0.0f;
        bool  particlesPlaying_ = false;
        // WindowLifeCycle が起動時に Config から入れたライト色。ここへ戻すのが「晴れ」
        glm::vec3 clearLightColor_ = glm::vec3(1.0f, 1.0f, 1.0f);
        std::mt19937 random_{std::random_device{}()};

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(skyDomeUpper_));
            archive(CEREAL_NVP(skyDomeLower_));
            archive(CEREAL_NVP(upperRotator_));
            archive(CEREAL_NVP(lowerRotator_));
            archive(CEREAL_NVP(flashRenderer_));
            archive(CEREAL_NVP(wispParticle_));
            archive(CEREAL_NVP(gustParticle_));
            archive(CEREAL_NVP(thunderNearSound_));
            archive(CEREAL_NVP(thunderFarSound1_));
            archive(CEREAL_NVP(thunderFarSound2_));
            archive(CEREAL_NVP(clearSkyTint_));
            archive(CEREAL_NVP(stormSkyTint_));
            archive(CEREAL_NVP(clearUpperRotateSpeedDeg_));
            archive(CEREAL_NVP(stormUpperRotateSpeedDeg_));
            archive(CEREAL_NVP(clearLowerRotateSpeedDeg_));
            archive(CEREAL_NVP(stormLowerRotateSpeedDeg_));
            archive(CEREAL_NVP(stormFogColor_));
            archive(CEREAL_NVP(clearFogStart_));
            archive(CEREAL_NVP(clearFogEnd_));
            archive(CEREAL_NVP(stormFogStart_));
            archive(CEREAL_NVP(stormFogEnd_));
            archive(CEREAL_NVP(stormLightColor_));
            archive(CEREAL_NVP(maxSustainShake_));
            archive(CEREAL_NVP(particlePlayThreshold_));
            archive(CEREAL_NVP(flashMaxBlendRate_));
            archive(CEREAL_NVP(lightningLightBoost_));
            archive(CEREAL_NVP(thunderDelay_secs_));
            archive(CEREAL_NVP(firstFlashDecay_secs_));
            archive(CEREAL_NVP(secondFlashDelay_secs_));
            archive(CEREAL_NVP(secondFlashDecay_secs_));
            archive(CEREAL_NVP(secondFlashStrength_));
            archive(CEREAL_NVP(skyFlashWeight_));
            archive(CEREAL_NVP(distantFlashStrength_));
            archive(CEREAL_NVP(distantFlashDuration_secs_));
            archive(CEREAL_NVP(distantThunderThreshold_));
            archive(CEREAL_NVP(distantThunderMinInterval_secs_));
            archive(CEREAL_NVP(distantThunderMaxInterval_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(skyDomeUpper_));
            if (version >= 0) archive(CEREAL_NVP(skyDomeLower_));
            if (version >= 0) archive(CEREAL_NVP(upperRotator_));
            if (version >= 0) archive(CEREAL_NVP(lowerRotator_));
            if (version >= 0) archive(CEREAL_NVP(flashRenderer_));
            if (version >= 0) archive(CEREAL_NVP(wispParticle_));
            if (version >= 0) archive(CEREAL_NVP(gustParticle_));
            if (version >= 0) archive(CEREAL_NVP(thunderNearSound_));
            if (version >= 0) archive(CEREAL_NVP(thunderFarSound1_));
            if (version >= 0) archive(CEREAL_NVP(thunderFarSound2_));
            if (version >= 0) archive(CEREAL_NVP(clearSkyTint_));
            if (version >= 0) archive(CEREAL_NVP(stormSkyTint_));
            if (version >= 0) archive(CEREAL_NVP(clearUpperRotateSpeedDeg_));
            if (version >= 0) archive(CEREAL_NVP(stormUpperRotateSpeedDeg_));
            if (version >= 0) archive(CEREAL_NVP(clearLowerRotateSpeedDeg_));
            if (version >= 0) archive(CEREAL_NVP(stormLowerRotateSpeedDeg_));
            if (version >= 0) archive(CEREAL_NVP(stormFogColor_));
            if (version >= 0) archive(CEREAL_NVP(clearFogStart_));
            if (version >= 0) archive(CEREAL_NVP(clearFogEnd_));
            if (version >= 0) archive(CEREAL_NVP(stormFogStart_));
            if (version >= 0) archive(CEREAL_NVP(stormFogEnd_));
            if (version >= 0) archive(CEREAL_NVP(stormLightColor_));
            if (version >= 0) archive(CEREAL_NVP(maxSustainShake_));
            if (version >= 0) archive(CEREAL_NVP(particlePlayThreshold_));
            if (version >= 0) archive(CEREAL_NVP(flashMaxBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(lightningLightBoost_));
            if (version >= 0) archive(CEREAL_NVP(thunderDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(firstFlashDecay_secs_));
            if (version >= 0) archive(CEREAL_NVP(secondFlashDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(secondFlashDecay_secs_));
            if (version >= 0) archive(CEREAL_NVP(secondFlashStrength_));
            if (version >= 0) archive(CEREAL_NVP(skyFlashWeight_));
            if (version >= 0) archive(CEREAL_NVP(distantFlashStrength_));
            if (version >= 0) archive(CEREAL_NVP(distantFlashDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(distantThunderThreshold_));
            if (version >= 0) archive(CEREAL_NVP(distantThunderMinInterval_secs_));
            if (version >= 0) archive(CEREAL_NVP(distantThunderMaxInterval_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Weather::WeatherService, 0)

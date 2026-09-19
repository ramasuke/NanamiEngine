#include "WeatherService.h"

#include <algorithm>
#include <cmath>
#include "DxLib.h"
#include "../Sound/SoundPlayer.h"
#include "../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"

namespace GamePlay::Weather
{
    namespace
    {
        float Lerp(const float a, const float b, const float t) { return a + (b - a) * t; }
        glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, const float t) { return a + (b - a) * t; }
    }

    WeatherService* WeatherService::instance_ = nullptr;

    void WeatherService::SetStorm(const float targetIntensity, const float blendSeconds)
    {
        const float target = std::clamp(targetIntensity, 0.0f, 1.0f);
        //NOTE: BTのSequenceは毎フレーム子0から再Tickされるので、同じ目標の指定は無視して進行を二重に進めない
        if (target == stormTarget_)
            return;

        stormTarget_ = target;
        if (blendSeconds <= 0.0f)
        {
            stormIntensity_  = target;
            stormBlendSpeed_ = 0.0f;
            return;
        }
        stormBlendSpeed_ = std::abs(target - stormIntensity_) / blendSeconds;
    }

    void WeatherService::Lightning(const float intensity, const float durationSeconds)
    {
        Flash(intensity, durationSeconds);
        pendingThunder_secs_ = thunderDelay_secs_;
    }

    void WeatherService::Flash(const float intensity, const float durationSeconds)
    {
        if (durationSeconds <= 0.0f)
            return;

        lightningStrength_ = std::clamp((std::max)(intensity, LightningBrightness()), 0.0f, 1.0f);
        lightningDuration_ = durationSeconds;
        lightningElapsed_  = 0.0f;
    }

    void WeatherService::OnAwake()
    {
        instance_ = this;

        using Config = NanamiEngine::Core::Application::Configuration::AppConfiguration;
        clearLightColor_ = glm::vec3(Config::GetLightDifR(), Config::GetLightDifG(), Config::GetLightDifB());

        distantThunderTimer_secs_ = distantThunderMaxInterval_secs_;
        if (flashRenderer_)
            flashRenderer_->SetBlendRate(0);
    }

    void WeatherService::OnUpdate()
    {
        if (!IsEnable())
            return;

        const float deltaTime = Time::DeltaTime();
        UpdateIntensity     (deltaTime);
        UpdateLightning     (deltaTime);
        UpdatePendingThunder(deltaTime);
        UpdateDistantThunder(deltaTime);

        ApplySky      ();
        ApplyFog      ();
        ApplyLight    ();
        ApplyShake    ();
        ApplyParticles();
    }

    void WeatherService::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;

        RestoreClearWeather();
    }

    void WeatherService::UpdateIntensity(const float deltaTime)
    {
        if (stormIntensity_ == stormTarget_)
            return;

        const float step = stormBlendSpeed_ * deltaTime;
        stormIntensity_ = stormIntensity_ < stormTarget_
            ? (std::min)(stormIntensity_ + step, stormTarget_)
            : (std::max)(stormIntensity_ - step, stormTarget_);
    }

    void WeatherService::UpdateLightning(const float deltaTime)
    {
        if (lightningElapsed_ >= lightningDuration_)
            return;

        lightningElapsed_ += deltaTime;
    }

    void WeatherService::UpdatePendingThunder(const float deltaTime)
    {
        if (pendingThunder_secs_ < 0.0f)
            return;

        pendingThunder_secs_ -= deltaTime;
        if (pendingThunder_secs_ > 0.0f)
            return;

        pendingThunder_secs_ = -1.0f;
        if (thunderNearSound_)
            Sound::SoundPlayer::PlaySe(*thunderNearSound_.get(), Sound::SoundPlayer::Position());
    }

    void WeatherService::UpdateDistantThunder(const float deltaTime)
    {
        if (stormIntensity_ < distantThunderThreshold_)
            return;

        distantThunderTimer_secs_ -= deltaTime;
        if (distantThunderTimer_secs_ > 0.0f)
            return;

        std::uniform_real_distribution interval(distantThunderMinInterval_secs_, distantThunderMaxInterval_secs_);
        distantThunderTimer_secs_ = interval(random_);

        Flash(distantFlashStrength_, distantFlashDuration_secs_);

        std::uniform_int_distribution<int> pick(0, 1);
        const auto& sound = pick(random_) == 0 ? thunderFarSound1_ : thunderFarSound2_;
        if (sound)
            Sound::SoundPlayer::PlaySe(*sound.get(), Sound::SoundPlayer::Position());
    }

    float WeatherService::LightningBrightness() const
    {
        if (lightningElapsed_ >= lightningDuration_)
            return 0.0f;

        const float first  = std::exp(-lightningElapsed_ / firstFlashDecay_secs_);
        const float second = secondFlashStrength_
            * std::exp(-(std::max)(lightningElapsed_ - secondFlashDelay_secs_, 0.0f) / secondFlashDecay_secs_);
        return std::clamp((std::max)(first, second) * lightningStrength_, 0.0f, 1.0f);
    }

    void WeatherService::ApplySky() const
    {
        const glm::vec3 tint   = Lerp(clearSkyTint_, stormSkyTint_, stormIntensity_);
        const glm::vec3 litSky = Lerp(tint, glm::vec3(1.0f), LightningBrightness() * skyFlashWeight_);
        if (skyDomeUpper_) skyDomeUpper_->SetTint(litSky);
        if (skyDomeLower_) skyDomeLower_->SetTint(litSky);

        if (upperRotator_)
            upperRotator_->SetRotateSpeedDegPerSec(
                Lerp(clearUpperRotateSpeedDeg_, stormUpperRotateSpeedDeg_, stormIntensity_));
        if (lowerRotator_)
            lowerRotator_->SetRotateSpeedDegPerSec(
                Lerp(clearLowerRotateSpeedDeg_, stormLowerRotateSpeedDeg_, stormIntensity_));
    }

    void WeatherService::ApplyFog() const
    {
        if (stormIntensity_ <= 0.0f)
        {
            SetFogEnable(FALSE);
            return;
        }

        SetFogEnable(TRUE);
        SetFogColor(
            static_cast<int>(stormFogColor_.r * 255.0f),
            static_cast<int>(stormFogColor_.g * 255.0f),
            static_cast<int>(stormFogColor_.b * 255.0f));
        SetFogStartEnd(
            Lerp(clearFogStart_, stormFogStart_, stormIntensity_),
            Lerp(clearFogEnd_,   stormFogEnd_,   stormIntensity_));
    }

    void WeatherService::ApplyLight() const
    {
        const glm::vec3 base = Lerp(clearLightColor_, stormLightColor_, stormIntensity_);
        const glm::vec3 lit  = Lerp(base, clearLightColor_ * lightningLightBoost_, LightningBrightness());
        SetLightDifColor(GetColorF(lit.r, lit.g, lit.b, 1.0f));

        if (flashRenderer_)
            flashRenderer_->SetBlendRate(static_cast<int>(LightningBrightness() * flashMaxBlendRate_));
    }

    void WeatherService::ApplyShake() const
    {
        if (stormIntensity_ <= 0.0f || maxSustainShake_ <= 0.0f)
            return;

        CineMachine::Behaviour::ShakeCameraBehaviour::SustainShakeMainCamera(stormIntensity_ * maxSustainShake_);
    }

    void WeatherService::ApplyParticles()
    {
        const bool shouldPlay = stormIntensity_ >= particlePlayThreshold_;
        if (shouldPlay == particlesPlaying_)
            return;

        particlesPlaying_ = shouldPlay;
        if (!gustParticle_)
            return;

        if (shouldPlay) gustParticle_->Play();
        else            gustParticle_->Stop();
    }

    void WeatherService::RestoreClearWeather() const
    {
        SetFogEnable(FALSE);
        SetLightDifColor(GetColorF(clearLightColor_.r, clearLightColor_.g, clearLightColor_.b, 1.0f));
        if (skyDomeUpper_) skyDomeUpper_->SetTint(clearSkyTint_);
        if (skyDomeLower_) skyDomeLower_->SetTint(clearSkyTint_);
        if (flashRenderer_) flashRenderer_->SetBlendRate(0);
    }

    void WeatherService::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("skyDomeUpper_",     skyDomeUpper_    );
        ImGuiHelper::OnDrawInputField("skyDomeLower_",     skyDomeLower_    );
        ImGuiHelper::OnDrawInputField("upperRotator_",     upperRotator_    );
        ImGuiHelper::OnDrawInputField("lowerRotator_",     lowerRotator_    );
        ImGuiHelper::OnDrawInputField("flashRenderer_",    flashRenderer_   );
        ImGuiHelper::OnDrawInputField("gustParticle_",     gustParticle_    );
        ImGuiHelper::OnDrawInputField("thunderNearSound_", thunderNearSound_);
        ImGuiHelper::OnDrawInputField("thunderFarSound1_", thunderFarSound1_);
        ImGuiHelper::OnDrawInputField("thunderFarSound2_", thunderFarSound2_);
        ImGuiHelper::OnDrawInputField("clearSkyTint_",     clearSkyTint_    );
        ImGuiHelper::OnDrawInputField("stormSkyTint_",     stormSkyTint_    );
        ImGuiHelper::OnDrawInputField("clearUpperRotateSpeedDeg_", clearUpperRotateSpeedDeg_);
        ImGuiHelper::OnDrawInputField("stormUpperRotateSpeedDeg_", stormUpperRotateSpeedDeg_);
        ImGuiHelper::OnDrawInputField("clearLowerRotateSpeedDeg_", clearLowerRotateSpeedDeg_);
        ImGuiHelper::OnDrawInputField("stormLowerRotateSpeedDeg_", stormLowerRotateSpeedDeg_);
        ImGuiHelper::OnDrawInputField("stormFogColor_",    stormFogColor_   );
        ImGuiHelper::OnDrawInputField("clearFogStart_",    clearFogStart_   );
        ImGuiHelper::OnDrawInputField("clearFogEnd_",      clearFogEnd_     );
        ImGuiHelper::OnDrawInputField("stormFogStart_",    stormFogStart_   );
        ImGuiHelper::OnDrawInputField("stormFogEnd_",      stormFogEnd_     );
        ImGuiHelper::OnDrawInputField("stormLightColor_",  stormLightColor_ );
        ImGuiHelper::OnDrawInputField("maxSustainShake_",  maxSustainShake_ );
        ImGuiHelper::OnDrawInputField("particlePlayThreshold_", particlePlayThreshold_);
        ImGuiHelper::OnDrawInputField("flashMaxBlendRate_",     flashMaxBlendRate_    );
        ImGuiHelper::OnDrawInputField("lightningLightBoost_",   lightningLightBoost_  );
        ImGuiHelper::OnDrawInputField("thunderDelay_secs_",     thunderDelay_secs_    );
        ImGuiHelper::OnDrawInputField("firstFlashDecay_secs_",  firstFlashDecay_secs_ );
        ImGuiHelper::OnDrawInputField("secondFlashDelay_secs_", secondFlashDelay_secs_);
        ImGuiHelper::OnDrawInputField("secondFlashDecay_secs_", secondFlashDecay_secs_);
        ImGuiHelper::OnDrawInputField("secondFlashStrength_",   secondFlashStrength_  );
        ImGuiHelper::OnDrawInputField("skyFlashWeight_",        skyFlashWeight_       );
        ImGuiHelper::OnDrawInputField("distantFlashStrength_",      distantFlashStrength_     );
        ImGuiHelper::OnDrawInputField("distantFlashDuration_secs_", distantFlashDuration_secs_);
        ImGuiHelper::OnDrawInputField("distantThunderThreshold_",        distantThunderThreshold_);
        ImGuiHelper::OnDrawInputField("distantThunderMinInterval_secs_", distantThunderMinInterval_secs_);
        ImGuiHelper::OnDrawInputField("distantThunderMaxInterval_secs_", distantThunderMaxInterval_secs_);

        ImGui::Separator();
        ImGui::Text("stormIntensity_: %.3f -> %.3f", stormIntensity_, stormTarget_);
        if (ImGui::Button("Storm 1.0")) SetStorm(1.0f, 3.0f);
        ImGui::SameLine();
        if (ImGui::Button("Clear"))     SetStorm(0.0f, 3.0f);
        ImGui::SameLine();
        if (ImGui::Button("Lightning")) Lightning(1.0f, 0.45f);
    }
}

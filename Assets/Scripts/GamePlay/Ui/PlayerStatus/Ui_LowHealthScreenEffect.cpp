#include "Ui_LowHealthScreenEffect.h"

#include <algorithm>
#include <cmath>
#include "DxLib.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float MIN_VISIBLE_DANGER = 0.01f;
    }

    void LowHealthScreenEffect::Initialize(const GameCore::PlayerAvatar::IPlayerAvatarStatus& model)
    {
        maxHealth_  = model.MaxHealth();
        lastHealth_ = model.Health();
        OnChangeHealth(model.Health(), model.IsDeath());
        danger_ = isDowned_ ? 0.0f : CalcDanger(healthRate_);
        ResetDownedFade(isDowned_);

        subscription_.Dispose();
        // OnChangeHealth は Set 直後に流れるので、この時点の IsDeath は新しいHPを反映している
        subscription_.Set(model.OnChangeHealth().Subscribe([this, &model](const GameCore::StatusParameter::Health health)
            {
                OnChangeHealth(health, model.IsDeath());
            }));
    }

    void LowHealthScreenEffect::OnAwake()
    {
        colorGrade_ = Components().Catch<NanamiUi::ScreenColorGradeRenderer>().lock();
        ResetDownedFade(false);
    }

    void LowHealthScreenEffect::OnDestroy()
    {
        subscription_.Dispose();
    }

    void LowHealthScreenEffect::OnUpdate()
    {
        const float deltaTime  = Time::DeltaTime();
        const float healthRate = debugOverrideHealthRate_ >= 0.0f ? debugOverrideHealthRate_ : healthRate_;
        const bool  isDowned   = debugForceDowned_ || isDowned_;

        const float targetDanger = isDowned ? 0.0f : CalcDanger(healthRate);
        danger_ += (targetDanger - danger_) * (1.0f - std::exp(-deltaTime / (std::max)(dangerSmooth_secs_, 0.001f)));
        if (isDowned)
            downedFade_.PlayForward();
        else
            downedFade_.PlayBackward();
        downedFade_.Tick(deltaTime);
        const float downedWeight = downedFade_.Value();

        const float bpm = std::lerp(minBpm_, maxBpm_, danger_);
        sinceBeat_secs_ += deltaTime;
        if (!isDowned && danger_ > MIN_VISIBLE_DANGER && sinceBeat_secs_ >= 60.0f / (std::max)(bpm, 1.0f))
        {
            sinceBeat_secs_ = 0.0f;
            PlayHeartbeat(danger_);
        }

        const float awakeWeight = 1.0f - downedWeight;
        if (vignette_)
        {
            const float blendRate = danger_ * (static_cast<float>(vignetteBlendRate_) + static_cast<float>(pulseAddBlendRate_) * CalcPulse(sinceBeat_secs_));
            vignette_->SetBlendRate(static_cast<int>((std::min)(blendRate, 255.0f) * awakeWeight));
        }
        if (colorGrade_)
        {
            colorGrade_->SetSaturation(static_cast<int>(
                -danger_ * static_cast<float>(maxDesaturation_) * awakeWeight
                - static_cast<float>(downedDesaturation_) * downedWeight));
            colorGrade_->SetBright(static_cast<int>(-static_cast<float>(downedDarken_) * downedWeight));
        }
    }

    void LowHealthScreenEffect::OnChangeHealth(const GameCore::StatusParameter::Health& health, const bool isDeath)
    {
        healthRate_ = maxHealth_.Value() > 0 ? health / maxHealth_ : 0.0f;
        isDowned_   = isDeath;

        // 被弾と同時に1拍打たせ、鼓動の位相を被弾に揃える
        const float targetDanger = CalcDanger(healthRate_);
        if (health < lastHealth_ && !isDowned_ && targetDanger > MIN_VISIBLE_DANGER)
        {
            sinceBeat_secs_ = 0.0f;
            PlayHeartbeat((std::max)(danger_, targetDanger));
        }
        lastHealth_ = health;
    }

    void LowHealthScreenEffect::ResetDownedFade(const bool isDowned)
    {
        downedFade_.Set(tweeny::from(0.0f).to(1.0f).during(LibCore::Tween::Ms(downedFade_secs_)));
        if (isDowned)
            downedFade_.Complete();
    }

    void LowHealthScreenEffect::PlayHeartbeat(const float danger) const
    {
        if (!heartbeatSound_)
            return;

        const int handle = heartbeatSound_->GetDxLibHandle();
        if (handle == -1)
            return;

        const float volume = std::lerp(static_cast<float>(heartbeatMinVolume_), static_cast<float>(heartbeatMaxVolume_), std::clamp(danger, 0.0f, 1.0f));
        ChangeNextPlayVolumeSoundMem(std::clamp(static_cast<int>(volume), 0, 255), handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK, TRUE);
    }

    float LowHealthScreenEffect::CalcDanger(const float healthRate) const
    {
        const float range = startHealthRate_ - criticalHealthRate_;
        if (range <= 0.0f)
            return healthRate <= criticalHealthRate_ ? 1.0f : 0.0f;

        return std::clamp((startHealthRate_ - healthRate) / range, 0.0f, 1.0f);
    }

    float LowHealthScreenEffect::CalcPulse(const float sinceBeat_secs) const
    {
        const float decay_secs = (std::max)(pulseDecay_secs_, 0.001f);
        float pulse = std::exp(-sinceBeat_secs / decay_secs);
        if (sinceBeat_secs >= dubDelay_secs_)
            pulse += dubPulseStrength_ * std::exp(-(sinceBeat_secs - dubDelay_secs_) / decay_secs);
        return (std::min)(pulse, 1.0f);
    }

    void LowHealthScreenEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("vignette_", vignette_);
        ImGuiHelper::OnDrawInputField("startHealthRate_",    startHealthRate_);
        ImGuiHelper::OnDrawInputField("criticalHealthRate_", criticalHealthRate_);
        ImGuiHelper::OnDrawInputField("dangerSmooth_secs_",  dangerSmooth_secs_);
        ImGuiHelper::OnDrawInputField("vignetteBlendRate_",  vignetteBlendRate_);
        ImGuiHelper::OnDrawInputField("pulseAddBlendRate_",  pulseAddBlendRate_);
        ImGuiHelper::OnDrawInputField("minBpm_",             minBpm_);
        ImGuiHelper::OnDrawInputField("maxBpm_",             maxBpm_);
        ImGuiHelper::OnDrawInputField("dubDelay_secs_",      dubDelay_secs_);
        ImGuiHelper::OnDrawInputField("maxDesaturation_",    maxDesaturation_);
        ImGuiHelper::OnDrawInputField("downedDesaturation_", downedDesaturation_);
        ImGuiHelper::OnDrawInputField("downedDarken_",       downedDarken_);
        ImGuiHelper::OnDrawInputField("downedFade_secs_",    downedFade_secs_);
        ImGuiHelper::OnDrawInputField("heartbeatSound_",     heartbeatSound_);
        ImGuiHelper::OnDrawInputField("heartbeatMinVolume_", heartbeatMinVolume_);
        ImGuiHelper::OnDrawInputField("heartbeatMaxVolume_", heartbeatMaxVolume_);
        ImGuiHelper::OnDrawInputField("pulseDecay_secs_",    pulseDecay_secs_);
        ImGuiHelper::OnDrawInputField("dubPulseStrength_",   dubPulseStrength_);

        ImGui::Separator();
        ImGui::SliderFloat("debugOverrideHealthRate_ (-1 = off)", &debugOverrideHealthRate_, -1.0f, 1.0f);
        ImGui::Checkbox("debugForceDowned_", &debugForceDowned_);
        ImGui::Text("healthRate_: %.2f  danger_: %.2f  downedWeight_: %.2f", healthRate_, danger_, downedFade_.Value());
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LowHealthScreenEffect);
#pragma endregion

#include "Ui_BossHealthGauge.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Engine/Core/Application/Time/Time.h"
#include "../GaugeEffects/Ui_GaugeEffects.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void BossHealthGauge::Show(const std::string& bossName)
    {
        if (bossNameText_)
            bossNameText_->SetText(bossName);

        value_          = 0.0f;
        pulseTime_secs_ = 0.0f;
        introFill_.Play(tweeny::from(0.0f).to(1.0f).during(LibCore::Tween::Ms(introFillDuration_secs_)));
        for (const auto& shard : shards_)
        {
            if (!shard)
                continue;

            shard->SetValue(0.0f);
            if (const auto effects = shard->Components().Catch<GaugeEffects>().lock())
                effects->SnapTrail();
        }
        ApplyToRenderers();
        Entity().lock()->SetEnable(true);
    }

    void BossHealthGauge::SetHealthRate(const float healthRate)
    {
        targetRate_ = std::clamp(healthRate, 0.0f, 1.0f);
        if (!IsIntroPlaying())
            value_ = targetRate_;
    }

    void BossHealthGauge::ApplyToRenderers()
    {
        const bool isDanger   = IsDanger();
        const auto fillSprite = (isDanger && fillDangerSprite_ ? fillDangerSprite_ : fillSprite_).get();
        const float count     = static_cast<float>(shards_.size());
        for (std::size_t i = 0; i < shards_.size(); ++i)
        {
            const auto& shard = shards_[i];
            if (!shard)
                continue;

            shard->SetValue(std::clamp(value_ * count - static_cast<float>(i), 0.0f, 1.0f));
            if (!fillSprite)
                continue;

            if (const auto effects = shard->Components().Catch<GaugeEffects>().lock())
                effects->ChangeGaugeSprite(fillSprite);
            else
                shard->SetGaugeSprite(fillSprite);
        }

        if (crestGlow_)
        {
            const float wave = 0.5f + 0.5f * std::sin(pulseTime_secs_ * pulseFrequency_hz_ * 2.0f * std::numbers::pi_v<float>);
            const int blendRate = isDanger
                ? static_cast<int>(static_cast<float>(std::clamp(pulseMaxAlpha_, 0, 255)) * wave)
                : 0;
            crestGlow_->SetBlendRate(blendRate);
        }
    }

    bool BossHealthGauge::IsIntroPlaying() const
    {
        return introFill_.IsPlaying();
    }

    bool BossHealthGauge::IsDanger() const
    {
        return value_ > 0.0f && value_ <= dangerHealthRate_;
    }

    void BossHealthGauge::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();

        if (IsIntroPlaying())
        {
            introFill_.Tick(deltaTime);
            value_ = std::min(targetRate_, introFill_.Value());
        }

        pulseTime_secs_ = IsDanger() ? pulseTime_secs_ + deltaTime : 0.0f;

        ApplyToRenderers();
    }

    void BossHealthGauge::OnDrawGui()
    {
        float previewRate = targetRate_;
        if (ImGui::SliderFloat("healthRate (preview)", &previewRate, 0.0f, 1.0f))
        {
            SetHealthRate(previewRate);
            ApplyToRenderers();
        }
        ImGui::Text("value_: %.3f  shards: %d", value_, static_cast<int>(shards_.size()));

        ImGuiHelper::OnDrawInputField("bossNameText_", bossNameText_);
        ImGuiHelper::OnDrawInputField("shards_", shards_, [this]
        {
            if (ImGui::Button("Add Shard"))
            {
                shards_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("crestGlow_", crestGlow_);
        ImGuiHelper::OnDrawInputField("fillSprite_", fillSprite_);
        ImGuiHelper::OnDrawInputField("fillDangerSprite_", fillDangerSprite_);
        ImGuiHelper::OnDrawInputField("dangerHealthRate_", dangerHealthRate_);
        ImGuiHelper::OnDrawInputField("pulseFrequency_hz_", pulseFrequency_hz_);
        ImGuiHelper::OnDrawInputField("pulseMaxAlpha_", pulseMaxAlpha_);
        ImGuiHelper::OnDrawInputField("introFillDuration_secs_", introFillDuration_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BossHealthGauge);
#pragma endregion

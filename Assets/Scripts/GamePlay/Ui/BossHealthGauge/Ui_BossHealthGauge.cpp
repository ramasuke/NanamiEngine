#include "Ui_BossHealthGauge.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Ui
{
    void BossHealthGauge::Show(const std::string& bossName)
    {
        CatchParts();
        if (bossNameText_)
            bossNameText_->SetText(bossName);

        value_               = 0.0f;
        trailValue_          = 0.0f;
        trailWaitTimer_secs_ = 0.0f;
        introElapsed_secs_   = 0.0f;
        pulseTime_secs_      = 0.0f;
        ApplyToRenderers();
        Entity().lock()->SetEnable(true);
    }

    void BossHealthGauge::SetHealthRate(const float healthRate)
    {
        targetRate_ = std::clamp(healthRate, 0.0f, 1.0f);
        if (!IsIntroPlaying())
            ApplyValue(targetRate_);
    }

    void BossHealthGauge::CatchParts()
    {
        if (!shards_.empty() || !shardsObject_)
            return;
        for (const auto& child : shardsObject_->Transform().GetChildren())
        {
            if (auto shard = child->Components().Catch<BossHealthShardRenderer>(); !shard.expired())
                shards_.push_back(std::move(shard));
        }
    }

    void BossHealthGauge::ApplyValue(const float healthRate)
    {
        if (healthRate < value_)
        {
            trailValue_          = std::max(trailValue_, value_);
            trailWaitTimer_secs_ = trailDelay_secs_;
        }
        else
        {
            trailValue_ = healthRate;
        }
        value_ = healthRate;
    }

    void BossHealthGauge::ApplyToRenderers()
    {
        CatchParts();

        const bool isDanger = IsDanger();
        const float count   = static_cast<float>(shards_.size());
        for (std::size_t i = 0; i < shards_.size(); ++i)
        {
            const auto shard = shards_[i].lock();
            if (!shard)
                continue;

            const float index = static_cast<float>(i);
            shard->SetFill(
                std::clamp(value_      * count - index, 0.0f, 1.0f),
                std::clamp(trailValue_ * count - index, 0.0f, 1.0f));
            shard->SetDanger(isDanger);
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
        return introElapsed_secs_ < introFillDuration_secs_;
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
            introElapsed_secs_ += deltaTime;
            const float introRate = introFillDuration_secs_ > 0.0f ? std::min(1.0f, introElapsed_secs_ / introFillDuration_secs_) : 1.0f;
            value_      = std::min(targetRate_, introRate);
            trailValue_ = value_;
        }

        if (trailWaitTimer_secs_ > 0.0f)
            trailWaitTimer_secs_ = std::max(0.0f, trailWaitTimer_secs_ - deltaTime);
        else
            trailValue_ = std::max(value_, trailValue_ - trailSpeed_perSec_ * deltaTime);

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
        ImGui::Text("value_: %.3f  trailValue_: %.3f  shards: %d", value_, trailValue_, static_cast<int>(shards_.size()));

        ImGuiHelper::OnDrawInputField("bossNameText_", bossNameText_);
        ImGuiHelper::OnDrawInputField("shardsObject_", shardsObject_);
        ImGuiHelper::OnDrawInputField("crestGlow_", crestGlow_);
        ImGuiHelper::OnDrawInputField("dangerHealthRate_", dangerHealthRate_);
        ImGuiHelper::OnDrawInputField("trailDelay_secs_", trailDelay_secs_);
        ImGuiHelper::OnDrawInputField("trailSpeed_perSec_", trailSpeed_perSec_);
        ImGuiHelper::OnDrawInputField("pulseFrequency_hz_", pulseFrequency_hz_);
        ImGuiHelper::OnDrawInputField("pulseMaxAlpha_", pulseMaxAlpha_);
        ImGuiHelper::OnDrawInputField("introFillDuration_secs_", introFillDuration_secs_);
    }
}

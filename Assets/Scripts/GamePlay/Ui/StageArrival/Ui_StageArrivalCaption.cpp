#include "Ui_StageArrivalCaption.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void StageArrivalCaption::ShowIsland(const std::string& title, const std::string& subtitle)
    {
        if (const auto text = islandTitle_.get())    text->SetText(title);
        if (const auto text = islandSubtitle_.get()) text->SetText(subtitle);
        island_.target   = 1.0f;
        landmark_.target = 0.0f;
        isDestroyRequested_ = false;
        ApplyIsland();
    }

    void StageArrivalCaption::ShowLandmark(const std::string& title, const std::string& subtitle)
    {
        if (const auto text = landmarkTitle_.get())    text->SetText(title);
        if (const auto text = landmarkSubtitle_.get()) text->SetText(subtitle);
        island_.target   = 0.0f;
        landmark_.target = 1.0f;
        isDestroyRequested_ = false;
        ApplyLandmark();
    }

    void StageArrivalCaption::Hide()
    {
        island_.target   = 0.0f;
        landmark_.target = 0.0f;
    }

    void StageArrivalCaption::HideAndDestroy()
    {
        Hide();
        isDestroyRequested_ = true;
    }

    void StageArrivalCaption::OnUpdate()
    {
        const float deltaSecs = Time::DeltaTime();
        StepCard(island_,   deltaSecs, fadeIn_secs_, fadeOut_secs_);
        StepCard(landmark_, deltaSecs, fadeIn_secs_, fadeOut_secs_);
        ApplyIsland();
        ApplyLandmark();

        if (isDestroyRequested_ && island_.rate <= 0.0f && landmark_.rate <= 0.0f)
        {
            isDestroyRequested_ = false;
            if (const auto entity = Entity().lock())
                entity->OnDestroy();
        }
    }

    void StageArrivalCaption::StepCard(Card& card, const float deltaSecs, const float fadeIn_secs, const float fadeOut_secs)
    {
        const float duration_secs = card.target > card.rate ? fadeIn_secs : fadeOut_secs;
        const float step = duration_secs > 0.0f ? deltaSecs / duration_secs : 1.0f;
        card.rate = card.target > card.rate
            ? (std::min)(card.rate + step, card.target)
            : (std::max)(card.rate - step, card.target);
    }

    int StageArrivalCaption::ToBlendRate(const float rate, const int maxBlendRate)
    {
        // なめらかに立ち上げて、なめらかに消す
        const float eased = rate * rate * (3.0f - 2.0f * rate);
        return std::clamp(static_cast<int>(static_cast<float>(maxBlendRate) * eased), 0, 255);
    }

    void StageArrivalCaption::ApplyIsland() const
    {
        const int blendRate = ToBlendRate(island_.rate, 255);
        if (const auto image = islandVignette_  .get()) image->SetBlendRate(ToBlendRate(island_.rate, vignetteBlendRate_));
        if (const auto image = islandRuleTop_   .get()) image->SetBlendRate(blendRate);
        if (const auto image = islandRuleBottom_.get()) image->SetBlendRate(blendRate);
        if (const auto text  = islandTitle_     .get()) text ->SetBlendRate(blendRate);
        if (const auto text  = islandSubtitle_  .get()) text ->SetBlendRate(blendRate);
    }

    void StageArrivalCaption::ApplyLandmark() const
    {
        const int blendRate = ToBlendRate(landmark_.rate, 255);
        if (const auto image = landmarkBand_    .get()) image->SetBlendRate(ToBlendRate(landmark_.rate, bandBlendRate_));
        if (const auto image = landmarkRule_    .get()) image->SetBlendRate(blendRate);
        if (const auto text  = landmarkTitle_   .get()) text ->SetBlendRate(blendRate);
        if (const auto text  = landmarkSubtitle_.get()) text ->SetBlendRate(blendRate);
    }

    void StageArrivalCaption::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("islandVignette_", islandVignette_);
        ImGuiHelper::OnDrawInputField("islandRuleTop_", islandRuleTop_);
        ImGuiHelper::OnDrawInputField("islandRuleBottom_", islandRuleBottom_);
        ImGuiHelper::OnDrawInputField("islandTitle_", islandTitle_);
        ImGuiHelper::OnDrawInputField("islandSubtitle_", islandSubtitle_);
        ImGuiHelper::OnDrawInputField("landmarkBand_", landmarkBand_);
        ImGuiHelper::OnDrawInputField("landmarkRule_", landmarkRule_);
        ImGuiHelper::OnDrawInputField("landmarkTitle_", landmarkTitle_);
        ImGuiHelper::OnDrawInputField("landmarkSubtitle_", landmarkSubtitle_);
        ImGuiHelper::OnDrawInputField("fadeIn_secs_", fadeIn_secs_);
        ImGuiHelper::OnDrawInputField("fadeOut_secs_", fadeOut_secs_);
        ImGuiHelper::OnDrawInputField("vignetteBlendRate_", vignetteBlendRate_);
        ImGuiHelper::OnDrawInputField("bandBlendRate_", bandBlendRate_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageArrivalCaption);
#pragma endregion

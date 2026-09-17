#include "Ui_SwordMan_ActionInstructTutorial.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/Status/ControlGuideFocus/SwordMan_IControlGuideFocusRequest.h"

namespace GamePlay::Ui
{
    namespace
    {
        float TutorialMoveTowards(const float current, const float target, const float maxDelta)
        {
            if (current < target)
                return std::min(current + maxDelta, target);
            return std::max(current - maxDelta, target);
        }

        float TutorialStepRate(const float deltaTime, const float duration_secs)
        {
            return duration_secs > 0.0f ? deltaTime / duration_secs : 1.0f;
        }

        float TutorialEaseOutBack(const float rate)
        {
            constexpr float OVERSHOOT = 1.70158f;
            const float t = std::clamp(rate, 0.0f, 1.0f) - 1.0f;
            return 1.0f + (OVERSHOOT + 1.0f) * t * t * t + OVERSHOOT * t * t;
        }

        int TutorialBlendRate(const float alpha)
        {
            return std::clamp(static_cast<int>(alpha), 0, 255);
        }
    }

    void SwordManActionInstructTutorial::Initialize(GameCore::PlayerAvatar::SwordMan::IControlGuideFocusRequest& guideFocus)
    {
        guideFocus_ = &guideFocus;
    }

    void SwordManActionInstructTutorial::CatchParts()
    {
        if (isPartsCaught_)
            return;
        isPartsCaught_ = true;

        if (card_)
            cardBasePos_ = card_->Transform().GetLocalPos();
    }

    std::string SwordManActionInstructTutorial::StepLabel(const std::size_t stepIndex) const
    {
        return stepLabelPrefix_ + " " + std::to_string(stepIndex + 1) + " / " + std::to_string(steps_.size());
    }

    void SwordManActionInstructTutorial::ShowStep(const std::size_t stepIndex)
    {
        if (stepIndex >= steps_.size())
            return;

        CatchParts();

        const bool wasHidden = !isShown_;
        stepIndex_ = stepIndex;
        isShown_   = true;
        isCleared_ = false;
        clearRate_ = 0.0f;
        textRate_  = 0.0f;

        if (const auto entity = Entity().lock())
            entity->SetEnable(true);

        if (stepText_)  stepText_ ->SetText(StepLabel(stepIndex));
        if (titleText_) titleText_->SetText(steps_[stepIndex].Title());
        if (bodyText_)  bodyText_ ->SetText(steps_[stepIndex].Body ());
        if (clearText_) clearText_->SetText(clearedText_);

        // 初めて出すときだけ、指された行の高さへ飛ばしておく
        if (wasHidden)
        {
            const std::optional<glm::vec2> anchor = guideFocus_ ? guideFocus_->FocusAnchor() : std::optional<glm::vec2>{};
            cardPos_px_ = anchor ? *anchor + anchorOffset_px_ : fallbackPos_px_;
        }
    }

    Coroutine::Task<void> SwordManActionInstructTutorial::PlayClearedAsync()
    {
        isCleared_ = true;
        co_await Coroutine::WaitForSeconds(clearHoldDuration_secs_);
    }

    void SwordManActionInstructTutorial::Hide()
    {
        isShown_ = false;
    }

    void SwordManActionInstructTutorial::OnUpdate()
    {
        CatchParts();

        const float deltaTime = Time::DeltaTime();
        appearRate_ = TutorialMoveTowards(appearRate_, isShown_ ? 1.0f : 0.0f, TutorialStepRate(deltaTime, appearDuration_secs_));
        textRate_   = TutorialMoveTowards(textRate_  , isShown_ ? 1.0f : 0.0f, TutorialStepRate(deltaTime, textFadeDuration_secs_));
        clearRate_  = TutorialMoveTowards(clearRate_ , isCleared_ ? 1.0f : 0.0f, TutorialStepRate(deltaTime, clearPopDuration_secs_));

        const std::optional<glm::vec2> anchor = guideFocus_ ? guideFocus_->FocusAnchor() : std::optional<glm::vec2>{};
        const glm::vec2 target = anchor ? *anchor + anchorOffset_px_ : fallbackPos_px_;
        const float follow = anchorFollowSpeed_pxPerSec_ * deltaTime;
        cardPos_px_ = glm::vec2(TutorialMoveTowards(cardPos_px_.x, target.x, follow),
                                TutorialMoveTowards(cardPos_px_.y, target.y, follow));
        Transform().SetLocalPos(glm::vec3(cardPos_px_.x, cardPos_px_.y, 0.0f));

        PresentText();
        PresentFade();

        if (!isShown_ && appearRate_ <= 0.0f)
        {
            if (const auto entity = Entity().lock())
                entity->SetEnable(false);
        }
    }

    void SwordManActionInstructTutorial::PresentText() const
    {
        if (!card_)
            return;

        const float hidden = 1.0f - appearRate_;
        card_->Transform().SetLocalPos(cardBasePos_ + glm::vec3(appearSlide_px_ * hidden * hidden, 0.0f, 0.0f));

        if (clearMark_)
        {
            const float pop = TutorialEaseOutBack(clearRate_) * clearMarkPopScale_;
            clearMark_->Transform().SetLocalScale(glm::vec3(std::max(pop, 0.0f)));
        }
    }

    void SwordManActionInstructTutorial::PresentFade() const
    {
        const float panelAlpha = 255.0f * appearRate_ * bodyAlphaRate_;
        const float taskAlpha  = 255.0f * appearRate_ * textRate_ * (1.0f - clearRate_);
        const float clearAlpha = 255.0f * appearRate_ * clearRate_;

        if (panel_)         panel_        ->SetBlendRate(TutorialBlendRate(panelAlpha));
        if (tail_)          tail_         ->SetBlendRate(TutorialBlendRate(panelAlpha));
        if (accent_)        accent_       ->SetBlendRate(TutorialBlendRate(255.0f * appearRate_ * (1.0f - clearRate_)));
        if (accentCleared_) accentCleared_->SetBlendRate(TutorialBlendRate(clearAlpha));
        if (clearMark_)     clearMark_    ->SetBlendRate(TutorialBlendRate(clearAlpha));

        if (stepText_)  stepText_ ->SetBlendRate(TutorialBlendRate(255.0f * appearRate_ * textRate_));
        if (titleText_) titleText_->SetBlendRate(TutorialBlendRate(taskAlpha));
        if (bodyText_)  bodyText_ ->SetBlendRate(TutorialBlendRate(taskAlpha));
        if (clearText_) clearText_->SetBlendRate(TutorialBlendRate(clearAlpha));
    }

    void SwordManActionInstructTutorial::OnDrawGui()
    {
        ImGui::Text("step: %d / %d", static_cast<int>(stepIndex_ + 1), static_cast<int>(steps_.size()));
        ImGui::Text("appear %.2f  text %.2f  clear %.2f", appearRate_, textRate_, clearRate_);

        ImGuiHelper::OnDrawInputField("card_", card_);
        ImGuiHelper::OnDrawInputField("panel_", panel_);
        ImGuiHelper::OnDrawInputField("tail_", tail_);
        ImGuiHelper::OnDrawInputField("accent_", accent_);
        ImGuiHelper::OnDrawInputField("accentCleared_", accentCleared_);
        ImGuiHelper::OnDrawInputField("clearMark_", clearMark_);
        ImGuiHelper::OnDrawInputField("stepText_", stepText_);
        ImGuiHelper::OnDrawInputField("titleText_", titleText_);
        ImGuiHelper::OnDrawInputField("bodyText_", bodyText_);
        ImGuiHelper::OnDrawInputField("clearText_", clearText_);

        for (std::size_t i = 0; i < steps_.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            ImGui::SeparatorText(StepLabel(i).c_str());
            steps_[i].OnDrawGui();
            ImGui::PopID();
        }

        ImGuiHelper::OnDrawInputField("stepLabelPrefix_", stepLabelPrefix_);
        ImGuiHelper::OnDrawInputField("clearedText_", clearedText_);
        ImGuiHelper::OnDrawInputField("anchorOffset_px_", anchorOffset_px_);
        ImGuiHelper::OnDrawInputField("fallbackPos_px_", fallbackPos_px_);
        ImGuiHelper::OnDrawInputField("anchorFollowSpeed_pxPerSec_", anchorFollowSpeed_pxPerSec_);
        ImGuiHelper::OnDrawInputField("appearDuration_secs_", appearDuration_secs_);
        ImGuiHelper::OnDrawInputField("appearSlide_px_", appearSlide_px_);
        ImGuiHelper::OnDrawInputField("textFadeDuration_secs_", textFadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("clearPopDuration_secs_", clearPopDuration_secs_);
        ImGuiHelper::OnDrawInputField("clearHoldDuration_secs_", clearHoldDuration_secs_);
        ImGuiHelper::OnDrawInputField("clearMarkPopScale_", clearMarkPopScale_);
        ImGuiHelper::OnDrawInputField("bodyAlphaRate_", bodyAlphaRate_);
    }
}

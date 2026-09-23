#include "Ui_SwordMan_ActionInstructTutorial.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "../../../../Core/Game/PlayerAvatar/SwordMan/Status/ControlGuideFocus/SwordMan_IControlGuideFocusRequest.h"
#include "../../../Sound/UiSoundBank.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

        tweeny::tween<float> TutorialFadeTween(const float duration_secs)
        {
            return tweeny::from(0.0f).to(1.0f).during(LibCore::Tween::Ms(duration_secs));
        }

        void TutorialFade(LibCore::Tween::TweenPlayer<float>& fade, const bool isOn, const float deltaTime)
        {
            if (isOn)
                fade.PlayForward();
            else
                fade.PlayBackward();
            fade.Tick(deltaTime);
        }

        void TutorialResetFade(LibCore::Tween::TweenPlayer<float>& fade)
        {
            fade.PlayBackward();
            fade.Complete();
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

        appearFade_.Set(TutorialFadeTween(appearDuration_secs_));
        textFade_  .Set(TutorialFadeTween(textFadeDuration_secs_));
        clearFade_ .Set(TutorialFadeTween(clearPopDuration_secs_));
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
        TutorialResetFade(clearFade_);
        TutorialResetFade(textFade_);

        if (const auto entity = Entity().lock())
            entity->SetEnable(true);
        Sound::UiSoundBank::Play(Sound::UiSe::HudNotice);

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
        Sound::UiSoundBank::Play(Sound::UiSe::HudClear);
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
        TutorialFade(appearFade_, isShown_,   deltaTime);
        TutorialFade(textFade_,   isShown_,   deltaTime);
        TutorialFade(clearFade_,  isCleared_, deltaTime);

        const std::optional<glm::vec2> anchor = guideFocus_ ? guideFocus_->FocusAnchor() : std::optional<glm::vec2>{};
        const glm::vec2 target = anchor ? *anchor + anchorOffset_px_ : fallbackPos_px_;
        const float follow = anchorFollowSpeed_pxPerSec_ * deltaTime;
        cardPos_px_ = glm::vec2(TutorialMoveTowards(cardPos_px_.x, target.x, follow),
                                TutorialMoveTowards(cardPos_px_.y, target.y, follow));
        Transform().SetLocalPos(glm::vec3(cardPos_px_.x, cardPos_px_.y, 0.0f));

        PresentText();
        PresentFade();

        if (!isShown_ && appearFade_.Value() <= 0.0f)
        {
            if (const auto entity = Entity().lock())
                entity->SetEnable(false);
        }
    }

    void SwordManActionInstructTutorial::PresentText() const
    {
        if (!card_)
            return;

        const float hidden = 1.0f - appearFade_.Value();
        card_->Transform().SetLocalPos(cardBasePos_ + glm::vec3(appearSlide_px_ * hidden * hidden, 0.0f, 0.0f));

        if (clearMark_)
        {
            const float pop = LibCore::Tween::Ease(LibCore::EaseType::OutBack).Ease(clearFade_.Value()) * clearMarkPopScale_;
            clearMark_->Transform().SetLocalScale(glm::vec3(std::max(pop, 0.0f)));
        }
    }

    void SwordManActionInstructTutorial::PresentFade() const
    {
        const float appearRate = appearFade_.Value();
        const float textRate   = textFade_.Value();
        const float clearRate  = clearFade_.Value();
        const float panelAlpha = 255.0f * appearRate * bodyAlphaRate_;
        const float taskAlpha  = 255.0f * appearRate * textRate * (1.0f - clearRate);
        const float clearAlpha = 255.0f * appearRate * clearRate;

        if (panel_)         panel_        ->SetBlendRate(TutorialBlendRate(panelAlpha));
        if (tail_)          tail_         ->SetBlendRate(TutorialBlendRate(panelAlpha));
        if (accent_)        accent_       ->SetBlendRate(TutorialBlendRate(255.0f * appearRate * (1.0f - clearRate)));
        if (accentCleared_) accentCleared_->SetBlendRate(TutorialBlendRate(clearAlpha));
        if (clearMark_)     clearMark_    ->SetBlendRate(TutorialBlendRate(clearAlpha));

        if (stepText_)  stepText_ ->SetBlendRate(TutorialBlendRate(255.0f * appearRate * textRate));
        if (titleText_) titleText_->SetBlendRate(TutorialBlendRate(taskAlpha));
        if (bodyText_)  bodyText_ ->SetBlendRate(TutorialBlendRate(taskAlpha));
        if (clearText_) clearText_->SetBlendRate(TutorialBlendRate(clearAlpha));
    }

    void SwordManActionInstructTutorial::OnDrawGui()
    {
        ImGui::Text("step: %d / %d", static_cast<int>(stepIndex_ + 1), static_cast<int>(steps_.size()));
        ImGui::Text("appear %.2f  text %.2f  clear %.2f", appearFade_.Value(), textFade_.Value(), clearFade_.Value());

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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SwordManActionInstructTutorial);
#pragma endregion

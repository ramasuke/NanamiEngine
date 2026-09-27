#include "Ui_StageReturnNotice.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        using LibCore::EaseType;
        using LibCore::Tween::Ease;
        using LibCore::Tween::Ms;
    }

    void StageReturnNoticeUi::OnStart()
    {
        EnsureStarted();
    }

    void StageReturnNoticeUi::EnsureStarted()
    {
        if (isStarted_)
            return;
        isStarted_ = true;

        if (const auto notice = noticeRoot_.get())
            noticeBasePos_ = notice->Transform().GetLocalPos();
        if (phase_ == Phase::Hidden)
        {
            if (const auto root = visualRoot_.get())
                root->SetEnable(false);
        }
    }

    void StageReturnNoticeUi::Open(const bool isHostLeaving, const int selection)
    {
        EnsureStarted();
        if (const auto root = visualRoot_.get())
            root->SetEnable(true);
        if (const auto notice = soloNotice_.get())
            notice->SetEnable(!isHostLeaving);
        if (const auto notice = hostNotice_.get())
            notice->SetEnable(isHostLeaving);
        if (const auto note = hostNoteText_.get())
            note->SetEnable(isHostLeaving);

        phase_ = Phase::Entering;
        enterTween_.Play(tweeny::from(0.0f).to(1.0f).during(Ms(enterDuration_secs_)).via(Ease(EaseType::OutCubic)));
        UpdateEnter(0.0f);

        // 開いたときの行には判子を押し直さず、最初から押してある
        SetSelection(selection);
        stampScaleTween_.Stop();
        if (const auto stamp = pressingStamp_.lock())
        {
            stamp->Transform().SetLocalScale(stampBaseScale_);
            stamp->SetBlendRate(255);
        }
    }

    void StageReturnNoticeUi::Hide()
    {
        if (phase_ == Phase::Hidden)
            return;

        phase_ = Phase::Hidden;
        if (const auto stamp = pressingStamp_.lock(); stamp && stampScaleTween_.IsPlaying())
            stamp->Transform().SetLocalScale(stampBaseScale_);
        stampScaleTween_.Stop();
        if (const auto notice = noticeRoot_.get())
            notice->Transform().SetLocalPos(noticeBasePos_);
        if (const auto root = visualRoot_.get())
            root->SetEnable(false);
    }

    void StageReturnNoticeUi::SetSelection(const int index)
    {
        const bool isReturn = index == RETURN_INDEX;
        if (const auto label = returnLabel_.get())
            label->SetTextColor(isReturn ? selectedColor_ : unselectedColor_);
        if (const auto label = stayLabel_.get())
            label->SetTextColor(isReturn ? unselectedColor_ : selectedColor_);
        if (const auto underline = returnUnderline_.get())
            underline->SetEnable(isReturn);
        if (const auto underline = stayUnderline_.get())
            underline->SetEnable(!isReturn);
        if (const auto stamp = returnStamp_.get())
            stamp->SetEnable(false);
        if (const auto stamp = stayStamp_.get())
            stamp->SetEnable(false);

        PressStamp(isReturn ? returnStamp_.get() : stayStamp_.get());
    }

    void StageReturnNoticeUi::PressStamp(const std::shared_ptr<NanamiUi::BlendImageRenderer>& stamp)
    {
        // 押している途中の判子は大きさを戻してから次を押す
        if (const auto pressing = pressingStamp_.lock(); pressing && stampScaleTween_.IsPlaying())
            pressing->Transform().SetLocalScale(stampBaseScale_);
        stampScaleTween_.Stop();
        pressingStamp_ = stamp;
        if (!stamp)
            return;

        stamp->SetEnable(true);
        stamp->SetBlendRate(0);
        stampBaseScale_ = stamp->Transform().GetLocalScale();
        stampScaleTween_.Play(tweeny::from(stampStartScale_).to(1.0f)
            .during(Ms(stampDuration_secs_)).via(Ease(EaseType::OutCubic)));
        // 朱は押す時間の前半で乗り切る
        stampAlphaTween_.Play(tweeny::from(0.0f).to(1.0f).during(Ms(stampDuration_secs_ * 0.5f)));
        UpdateStamp(0.0f);
    }

    void StageReturnNoticeUi::OnUpdate()
    {
        if (phase_ == Phase::Hidden)
            return;

        const float deltaSecs = Time::DeltaTime();
        UpdateEnter(deltaSecs);
        UpdateStamp(deltaSecs);
    }

    void StageReturnNoticeUi::UpdateEnter(const float deltaSecs)
    {
        if (phase_ != Phase::Entering)
            return;

        const bool isFinished = enterTween_.Tick(deltaSecs);
        const float rate = enterTween_.Value();
        if (const auto notice = noticeRoot_.get())
            notice->Transform().SetLocalPos(noticeBasePos_ + glm::vec3(0.0f, -dropDistance_px_ * (1.0f - rate), 0.0f));
        ApplyVeil(rate);
        if (isFinished)
            phase_ = Phase::Shown;
    }

    void StageReturnNoticeUi::UpdateStamp(const float deltaSecs)
    {
        const auto stamp = pressingStamp_.lock();
        if (!stamp || !stampScaleTween_.IsPlaying())
            return;

        const bool isFinished = stampScaleTween_.Tick(deltaSecs);
        stampAlphaTween_.Tick(deltaSecs);
        stamp->Transform().SetLocalScale(stampBaseScale_ * stampScaleTween_.Value());
        stamp->SetBlendRate(static_cast<int>(255.0f * stampAlphaTween_.Value()));

        if (isFinished)
        {
            stamp->Transform().SetLocalScale(stampBaseScale_);
            stamp->SetBlendRate(255);
        }
    }

    void StageReturnNoticeUi::ApplyVeil(const float rate) const
    {
        const float clamped = std::clamp(rate, 0.0f, 1.0f);
        if (const auto veil = veil_.get())
            veil->SetBlendRate(static_cast<int>(static_cast<float>(veilBlendRate_) * clamped));
        if (const auto veil = veilBlack_.get())
            veil->SetBlendRate(static_cast<int>(static_cast<float>(veilBlackBlendRate_) * clamped));
    }

    void StageReturnNoticeUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("visualRoot_", visualRoot_);
        ImGuiHelper::OnDrawInputField("noticeRoot_", noticeRoot_);
        ImGuiHelper::OnDrawInputField("veilBlack_", veilBlack_);
        ImGuiHelper::OnDrawInputField("veil_", veil_);
        ImGuiHelper::OnDrawInputField("soloNotice_", soloNotice_);
        ImGuiHelper::OnDrawInputField("hostNotice_", hostNotice_);
        ImGuiHelper::OnDrawInputField("hostNoteText_", hostNoteText_);
        ImGuiHelper::OnDrawInputField("returnLabel_", returnLabel_);
        ImGuiHelper::OnDrawInputField("returnUnderline_", returnUnderline_);
        ImGuiHelper::OnDrawInputField("returnStamp_", returnStamp_);
        ImGuiHelper::OnDrawInputField("stayLabel_", stayLabel_);
        ImGuiHelper::OnDrawInputField("stayUnderline_", stayUnderline_);
        ImGuiHelper::OnDrawInputField("stayStamp_", stayStamp_);
        ImGuiHelper::OnDrawInputField("confirmButton_", confirmButton_);
        ImGuiHelper::OnDrawInputField("cancelButton_", cancelButton_);
        ImGuiHelper::OnDrawInputField("selectedColor_", selectedColor_);
        ImGuiHelper::OnDrawInputField("unselectedColor_", unselectedColor_);
        ImGuiHelper::OnDrawInputField("veilBlendRate_", veilBlendRate_);
        ImGuiHelper::OnDrawInputField("veilBlackBlendRate_", veilBlackBlendRate_);
        ImGuiHelper::OnDrawInputField("dropDistance_px_", dropDistance_px_);
        ImGuiHelper::OnDrawInputField("enterDuration_secs_", enterDuration_secs_);
        ImGuiHelper::OnDrawInputField("stampDuration_secs_", stampDuration_secs_);
        ImGuiHelper::OnDrawInputField("stampStartScale_", stampStartScale_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageReturnNoticeUi);
#pragma endregion

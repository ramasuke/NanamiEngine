#include "Ui_GameOverButton.h"

#include <algorithm>
#include <cmath>

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float GAME_OVER_BUTTON_TAU = 6.28318530718f;
    }

    void GameOverButton::OnAwake()
    {
        button_ = RequireComponent<NanamiUi::Button>();
        Apply();
    }

    void GameOverButton::SetAppearRate(const float appearRate)
    {
        appearRate_ = std::clamp(appearRate, 0.0f, 1.0f);
        Apply();
    }

    void GameOverButton::SetHighlighted(const bool isHighlighted)
    {
        if (isHighlighted_ == isHighlighted)
            return;

        isHighlighted_ = isHighlighted;
        // 選ばれた瞬間に熾火が一番明るいところから始まるようにする
        if (isHighlighted_)
            emberPhase_ = 0.25f;
    }

    void GameOverButton::Tick(const float deltaSecs)
    {
        const float target = isHighlighted_ ? 1.0f : 0.0f;
        const float step = highlightFadeSecs_ > 0.0f ? deltaSecs / highlightFadeSecs_ : 1.0f;
        highlight_ = highlight_ < target
            ? std::min(target, highlight_ + step)
            : std::max(target, highlight_ - step);

        emberPhase_ = std::fmod(emberPhase_ + deltaSecs * emberPulseHz_, 1.0f);
        Apply();
    }

    R4::Observable<NanamiUi::MouseState> GameOverButton::OnClick() const
    {
        return button_.lock()->OnClick();
    }

    R4::Observable<R4::Unit> GameOverButton::OnHover() const
    {
        return button_.lock()->OnHover();
    }

    void GameOverButton::Apply() const
    {
        const int alpha = static_cast<int>(255.0f * appearRate_);

        if (const auto plate = plate_.get())
            plate->SetBlendRate(static_cast<int>(alpha * (1.0f - highlight_)));
        if (const auto plateLit = plateLit_.get())
            plateLit->SetBlendRate(static_cast<int>(alpha * highlight_));
        if (const auto ember = ember_.get())
        {
            const float pulse = 0.5f + 0.5f * std::sin(emberPhase_ * GAME_OVER_BUTTON_TAU);
            const float emberRate = std::lerp(static_cast<float>(emberMinBlendRate_), static_cast<float>(emberMaxBlendRate_), pulse);
            ember->SetBlendRate(static_cast<int>(emberRate * highlight_ * appearRate_));
        }

        if (const auto label = label_.get())
        {
            // 札の絵のクロスフェードに紛れるので、文字色は半分を越えたところで切り替えるだけにする
            label->SetTextColor(highlight_ >= 0.5f ? labelLitColor_ : labelColor_);
            label->SetBlendRate(alpha);
        }
        if (const auto labelShadow = labelShadow_.get())
            labelShadow->SetBlendRate(alpha * 3 / 4);
    }

    void GameOverButton::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("plate_", plate_);
        ImGuiHelper::OnDrawInputField("plateLit_", plateLit_);
        ImGuiHelper::OnDrawInputField("ember_", ember_);
        ImGuiHelper::OnDrawInputField("label_", label_);
        ImGuiHelper::OnDrawInputField("labelShadow_", labelShadow_);
        ImGuiHelper::OnDrawInputField("labelColor_", labelColor_);
        ImGuiHelper::OnDrawInputField("labelLitColor_", labelLitColor_);
        ImGuiHelper::OnDrawInputField("highlightFadeSecs_", highlightFadeSecs_);
        ImGuiHelper::OnDrawInputField("emberPulseHz_", emberPulseHz_);
        ImGuiHelper::OnDrawInputField("emberMinBlendRate_", emberMinBlendRate_);
        ImGuiHelper::OnDrawInputField("emberMaxBlendRate_", emberMaxBlendRate_);
        ImGui::Text("highlight: %.2f  appear: %.2f", highlight_, appearRate_);
    }
}

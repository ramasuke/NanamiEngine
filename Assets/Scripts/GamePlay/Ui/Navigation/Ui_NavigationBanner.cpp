#include "Ui_NavigationBanner.h"

#include <algorithm>

#include "Ui_NavigationMemory.h"
#include "Ui_NavigationPresenter.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Platform/Draw2D/Draw2D.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        using LibCore::Dxlib::BlendMode;

        constexpr LibCore::Tween::EaseFunctor EASE_OUT_CUBIC{ LibCore::EaseType::OutCubic };

        float Rate(const float elapsed, const float duration)
        {
            return duration > 0.0f ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
        }
    }

    void NavigationBanner::OnUpdate()
    {
        auto& memory = NavigationMemory::Instance();
        const auto& current = memory.Current();

        // NOTE: 会話や演出の最中に目的が変わることが多いので、明けてから出す。出したら覚えて二度と出さない
        if (current && !current->title.empty() && current->title != memory.AnnouncedTitle() && !NavigationPresenter::IsQuiet())
        {
            title_        = current->title;
            elapsed_secs_ = 0.0f;
            isPlaying_    = true;
            memory.SetAnnouncedTitle(title_);
        }

        if (!isPlaying_)
            return;

        elapsed_secs_ += Time::DeltaTime();
        if (elapsed_secs_ >= fadeIn_secs_ + hold_secs_ + fadeOut_secs_)
            isPlaying_ = false;
    }

    float NavigationBanner::Alpha() const
    {
        if (!isPlaying_)
            return 0.0f;

        const float fadeIn  = Rate(elapsed_secs_, fadeIn_secs_);
        const float fadeOut = 1.0f - Rate(elapsed_secs_ - fadeIn_secs_ - hold_secs_, fadeOut_secs_);
        return std::min(EASE_OUT_CUBIC(fadeIn), fadeOut);
    }

    void NavigationBanner::DrawCenteredText(const std::string& text, const glm::vec2& centre, const float scale, const Color32& color, const float alpha) const
    {
        const int fontHandle = font_->DxLibHandle();
        const float width  = static_cast<float>(Platform::Draw2D::StringWidth(scale, text, fontHandle));
        const float height = static_cast<float>(Platform::Draw2D::FontSize(fontHandle)) * scale;

        Platform::Draw2D::SetBlendModeAlpha(BlendMode::Alpha, alpha);
        Platform::Draw2D::DrawString(glm::vec2(centre.x - width * 0.5f, centre.y - height * 0.5f), glm::vec2(scale, scale), text, color, fontHandle, font_->EdgeColor());
    }

    void NavigationBanner::OnUserInterfaceRender()
    {
        const float alpha = Alpha();
        if (!IsEnable() || alpha <= 0.0f || !font_ || NavigationPresenter::IsQuiet())
            return;

        const Platform::Draw2D::ScopedDrawState drawState;
        Platform::Draw2D::SetFilterMode(Platform::Draw2D::FilterMode::Bilinear);

        // 出てくるときだけ少し下から上がる
        const float rise   = riseDistance_px_ * (1.0f - EASE_OUT_CUBIC(Rate(elapsed_secs_, fadeIn_secs_)));
        const auto  screen = Platform::Draw2D::ScreenSize();
        const glm::vec2 centre(static_cast<float>(screen.x) * 0.5f, centerY_px_ + rise);

        if (bandSprite_)
        {
            Platform::Draw2D::SetBlendModeAlpha(BlendMode::Alpha, alpha * bandAlphaRate_);
            Platform::Draw2D::DrawRotaGraph(centre, 1.0, 0.0, bandSprite_->GetDxLibHandle());
        }

        DrawCenteredText(headingText_, centre + glm::vec2(0.0f, headingOffsetY_px_), headingScale_, headingColor_, alpha);
        DrawCenteredText(title_, centre + glm::vec2(0.0f, titleOffsetY_px_), titleScale_, titleColor_, alpha);
    }

    void NavigationBanner::OnDrawGui()
    {
        if (ImGui::Button("Replay (preview)"))
            NavigationMemory::Instance().SetAnnouncedTitle("");

        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("bandSprite_", bandSprite_);
        ImGuiHelper::OnDrawInputField("font_", font_);
        ImGuiHelper::OnDrawInputField("headingText_", headingText_);
        ImGuiHelper::OnDrawInputField("centerY_px_", centerY_px_);
        ImGuiHelper::OnDrawInputField("headingOffsetY_px_", headingOffsetY_px_);
        ImGuiHelper::OnDrawInputField("titleOffsetY_px_", titleOffsetY_px_);
        ImGuiHelper::OnDrawInputField("headingScale_", headingScale_);
        ImGuiHelper::OnDrawInputField("titleScale_", titleScale_);
        ImGuiHelper::OnDrawInputField("headingColor_", headingColor_);
        ImGuiHelper::OnDrawInputField("titleColor_", titleColor_);
        ImGuiHelper::OnDrawInputField("bandAlphaRate_", bandAlphaRate_);
        ImGuiHelper::OnDrawInputField("riseDistance_px_", riseDistance_px_);
        ImGuiHelper::OnDrawInputField("fadeIn_secs_", fadeIn_secs_);
        ImGuiHelper::OnDrawInputField("hold_secs_", hold_secs_);
        ImGuiHelper::OnDrawInputField("fadeOut_secs_", fadeOut_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::NavigationBanner);
#pragma endregion

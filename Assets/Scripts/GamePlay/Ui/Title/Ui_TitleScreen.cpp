#include "Ui_TitleScreen.h"

#include <algorithm>
#include <cmath>

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

        constexpr float TITLE_PI = 3.14159265f;

        /** @brief delaySecs を過ぎてから durationSecs で 0 → 1 */
        float TitleDelayedRate(const float elapsedSecs, const float delaySecs, const float durationSecs)
        {
            if (durationSecs <= 0.0f)
                return elapsedSecs >= delaySecs ? 1.0f : 0.0f;
            return std::clamp((elapsedSecs - delaySecs) / durationSecs, 0.0f, 1.0f);
        }

        int TitleBlend(const float rate) { return std::clamp(static_cast<int>(255.0f * rate), 0, 255); }

        void TitleSetBlend(const std::shared_ptr<NanamiUi::BlendImageRenderer>& image, const float rate)
        {
            if (image)
                image->SetBlendRate(TitleBlend(rate));
        }

        void TitleSetBlend(const std::shared_ptr<NanamiUi::TextRenderer>& text, const float rate)
        {
            if (text)
                text->SetBlendRate(TitleBlend(rate));
        }
    }

    void TitleScreenUi::OnStart()
    {
        for (int i = 0; i < MENU_COUNT; ++i)
        {
            if (const auto text = MenuText(i))
                menuBasePos_[static_cast<size_t>(i)] = text->Transform().GetLocalPos();
        }
        if (const auto band = selectBand_.get())
            bandBasePos_ = band->Transform().GetLocalPos();
        // NOTE: 文字の位置は上端、帯の位置は中心なので、置いたときの差を保って動かす
        bandOffsetY_ = bandBasePos_.y - menuBasePos_[START_INDEX].y;
        bandY_ = menuBasePos_[START_INDEX].y;

        lastTickMs_ = Time::NowMilliseconds();
        ApplyIntro();
        ApplyMenu(0.0f);
    }

    void TitleScreenUi::SkipIntro()
    {
        if (phase_ != Phase::Intro)
            return;

        introElapsed_secs_ = pressDelay_secs_ + pressFade_secs_;
        ApplyIntro();
        phase_ = Phase::PressWaiting;
        pressElapsed_secs_ = 0.0f;
    }

    void TitleScreenUi::ShowMenu()
    {
        SkipIntro();
        phase_ = Phase::Menu;
        isMenuShown_ = true;
        menuElapsed_secs_ = 0.0f;
    }

    void TitleScreenUi::HideMenu()
    {
        if (phase_ != Phase::Menu)
            return;

        phase_ = Phase::PressWaiting;
        isMenuShown_ = false;
        pressElapsed_secs_ = 0.0f;
    }

    void TitleScreenUi::SetSelection(const int index)
    {
        selection_ = std::clamp(index, 0, MENU_COUNT - 1);
    }

    void TitleScreenUi::SetStartLabel(const std::string& label) const
    {
        if (const auto text = startText_.get())
            text->SetText(label);
    }

    std::shared_ptr<NanamiUi::Button> TitleScreenUi::MenuButton(const int index) const
    {
        return index == START_INDEX ? startButton_.get() : exitButton_.get();
    }

    std::shared_ptr<NanamiUi::TextRenderer> TitleScreenUi::MenuText(const int index) const
    {
        return index == START_INDEX ? startText_.get() : exitText_.get();
    }

    void TitleScreenUi::OnUpdate()
    {
        const float deltaSecs = TickWallClockSeconds();

        if (phase_ == Phase::Intro)
        {
            introElapsed_secs_ += deltaSecs;
            ApplyIntro();
            if (introElapsed_secs_ >= pressDelay_secs_ + pressFade_secs_)
            {
                phase_ = Phase::PressWaiting;
                pressElapsed_secs_ = 0.0f;
            }
        }
        else
        {
            ApplyPress(deltaSecs);
        }

        ApplyMenu(deltaSecs);
    }

    float TitleScreenUi::TickWallClockSeconds()
    {
        const int nowMs = Time::NowMilliseconds();
        const float deltaSecs = static_cast<float>(nowMs - lastTickMs_) / 1000.0f;
        lastTickMs_ = nowMs;
        return std::clamp(deltaSecs, 0.0f, 0.25f);
    }

    void TitleScreenUi::ApplyIntro() const
    {
        const float veilRate = TitleDelayedRate(introElapsed_secs_, 0.0f, veilOpen_secs_);
        TitleSetBlend(veil_.get(), 1.0f - Ease(EaseType::SmoothStep).Ease(veilRate));

        const float logoRate = TitleDelayedRate(introElapsed_secs_, logoDelay_secs_, logoFade_secs_);
        TitleSetBlend(logo_.get(), Ease(EaseType::InOutSine).Ease(logoRate));

        const float pressRate = TitleDelayedRate(introElapsed_secs_, pressDelay_secs_, pressFade_secs_);
        TitleSetBlend(pressText_.get(), pressRate);
        TitleSetBlend(pressDeco_.get(), pressRate);
    }

    void TitleScreenUi::ApplyPress(const float deltaSecs)
    {
        pressElapsed_secs_ += deltaSecs;
        const float period = std::max(pressPulsePeriod_secs_, 0.1f);
        // 出し切った明るさから始めて、ゆっくり息をするように明滅させる
        const float wave = 0.5f + 0.5f * std::cos(2.0f * TITLE_PI * pressElapsed_secs_ / period);
        const float pulse = pressPulseMinRate_ + (1.0f - pressPulseMinRate_) * wave;
        const float visible = 1.0f - menuRate_;
        TitleSetBlend(pressText_.get(), pulse * visible);
        TitleSetBlend(pressDeco_.get(), visible);
        TitleSetBlend(logo_.get(), 1.0f);
        TitleSetBlend(veil_.get(), 0.0f);
    }

    void TitleScreenUi::ApplyMenu(const float deltaSecs)
    {
        if (phase_ == Phase::Menu)
            menuElapsed_secs_ += deltaSecs;

        const float fadeSecs = std::max(menuFade_secs_, 0.01f);
        const float totalSecs = fadeSecs + menuStagger_secs_ * static_cast<float>(MENU_COUNT - 1);
        const float step = deltaSecs / totalSecs;
        menuRate_ = std::clamp(menuRate_ + (isMenuShown_ ? step : -step), 0.0f, 1.0f);

        for (int i = 0; i < MENU_COUNT; ++i)
        {
            const float rowRate = std::clamp((menuRate_ * totalSecs - menuStagger_secs_ * static_cast<float>(i)) / fadeSecs, 0.0f, 1.0f);
            const float eased = Ease(EaseType::OutCubic).Ease(rowRate);
            const auto text = MenuText(i);
            if (!text)
                continue;

            // 右から少し滑り込ませる
            text->Transform().SetLocalPos(menuBasePos_[static_cast<size_t>(i)] + glm::vec3(menuSlide_px_ * (1.0f - eased), 0.0f, 0.0f));
            const float emphasis = i == selection_ ? 1.0f : unselectedTextRate_;
            TitleSetBlend(text, eased * emphasis);
        }

        // 帯は選んだ行へ指数的に追いかける
        const float targetY = menuBasePos_[static_cast<size_t>(selection_)].y;
        bandY_ += (targetY - bandY_) * std::clamp(bandFollowRate_ * deltaSecs, 0.0f, 1.0f);
        if (const auto band = selectBand_.get())
        {
            band->Transform().SetLocalPos(glm::vec3(bandBasePos_.x, bandY_ + bandOffsetY_, bandBasePos_.z));
            band->SetBlendRate(TitleBlend(menuRate_));
        }

        TitleSetBlend(moveHintTag_.get(), menuRate_);
        TitleSetBlend(moveHintText_.get(), menuRate_);
        TitleSetBlend(confirmHintTag_.get(), menuRate_);
        TitleSetBlend(confirmHintText_.get(), menuRate_);
    }

    void TitleScreenUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("veil_", veil_);
        ImGuiHelper::OnDrawInputField("logo_", logo_);
        ImGuiHelper::OnDrawInputField("pressText_", pressText_);
        ImGuiHelper::OnDrawInputField("pressDeco_", pressDeco_);
        ImGuiHelper::OnDrawInputField("startText_", startText_);
        ImGuiHelper::OnDrawInputField("exitText_", exitText_);
        ImGuiHelper::OnDrawInputField("startButton_", startButton_);
        ImGuiHelper::OnDrawInputField("exitButton_", exitButton_);
        ImGuiHelper::OnDrawInputField("selectBand_", selectBand_);
        ImGuiHelper::OnDrawInputField("moveHintTag_", moveHintTag_);
        ImGuiHelper::OnDrawInputField("moveHintText_", moveHintText_);
        ImGuiHelper::OnDrawInputField("confirmHintTag_", confirmHintTag_);
        ImGuiHelper::OnDrawInputField("confirmHintText_", confirmHintText_);
        ImGuiHelper::OnDrawInputField("veilOpen_secs_", veilOpen_secs_);
        ImGuiHelper::OnDrawInputField("logoDelay_secs_", logoDelay_secs_);
        ImGuiHelper::OnDrawInputField("logoFade_secs_", logoFade_secs_);
        ImGuiHelper::OnDrawInputField("pressDelay_secs_", pressDelay_secs_);
        ImGuiHelper::OnDrawInputField("pressFade_secs_", pressFade_secs_);
        ImGuiHelper::OnDrawInputField("pressPulsePeriod_secs_", pressPulsePeriod_secs_);
        ImGuiHelper::OnDrawInputField("pressPulseMinRate_", pressPulseMinRate_);
        ImGuiHelper::OnDrawInputField("menuFade_secs_", menuFade_secs_);
        ImGuiHelper::OnDrawInputField("menuStagger_secs_", menuStagger_secs_);
        ImGuiHelper::OnDrawInputField("menuSlide_px_", menuSlide_px_);
        ImGuiHelper::OnDrawInputField("menuInputGuard_secs_", menuInputGuard_secs_);
        ImGuiHelper::OnDrawInputField("bandFollowRate_", bandFollowRate_);
        ImGuiHelper::OnDrawInputField("unselectedTextRate_", unselectedTextRate_);
        ImGui::Text("phase: %d  menu: %.2f  selection: %d", static_cast<int>(phase_), menuRate_, selection_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::TitleScreenUi);
#pragma endregion

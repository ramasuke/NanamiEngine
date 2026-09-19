#include "Ui_GameOverScreen.h"

#include <algorithm>
#include <cmath>

#include "DxLib.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 石版が浮き上がり切るまでの割合。残りで落ちて着地する
        constexpr float GAME_OVER_SLAB_PEAK_RATE = 0.65f;

        /** @brief 開始からの経過を長さで割った 0..1。長さ 0 は即完了として扱う */
        float GameOverRate(const float sinceStartSecs, const float durationSecs)
        {
            if (durationSecs <= 0.0f)
                return sinceStartSecs >= 0.0f ? 1.0f : 0.0f;

            return std::clamp(sinceStartSecs / durationSecs, 0.0f, 1.0f);
        }

        float GameOverSmoothstep(const float rate) { return rate * rate * (3.0f - 2.0f * rate); }

        float GameOverEaseOutCubic(const float rate)
        {
            const float inv = 1.0f - rate;
            return 1.0f - inv * inv * inv;
        }

        /** @brief 下から勢いよく持ち上がって少し浮き、加速しながら落ちて止まる。戻り値は基準位置からの下向きのずれ */
        float GameOverSlabOffset(const float rate, const float riseDistance, const float overshoot)
        {
            if (rate < GAME_OVER_SLAB_PEAK_RATE)
            {
                const float up = GameOverEaseOutCubic(rate / GAME_OVER_SLAB_PEAK_RATE);
                return std::lerp(riseDistance, -overshoot, up);
            }

            const float fall = (rate - GAME_OVER_SLAB_PEAK_RATE) / (1.0f - GAME_OVER_SLAB_PEAK_RATE);
            return -overshoot * (1.0f - fall * fall);
        }

        int GameOverBlend(const float rate) { return std::clamp(static_cast<int>(255.0f * rate), 0, 255); }
    }

    void GameOverScreenUi::OnStart()
    {
        if (const auto slab = slab_.get())
            slabBasePos_ = slab->Transform().GetLocalPos();
        if (const auto retry = retryButton_.get())
            retryBasePos_ = retry->Transform().GetLocalPos();
        if (const auto title = titleButton_.get())
            titleBasePos_ = title->Transform().GetLocalPos();

        // 起動直後から出ていないように、常駐しているぶんを自分で畳んでおく
        SetVisualEnabled(false);
        lastTickMs_ = GetNowCount();
    }

    void GameOverScreenUi::Show()
    {
        phase_ = Phase::Intro;
        elapsedSecs_ = 0.0f;
        curtainElapsedSecs_ = 0.0f;
        isStingPlayed_ = false;
        isSlabLanded_ = false;
        lastTickMs_ = GetNowCount();

        SetVisualEnabled(true);
        SetSelection(RETRY_INDEX);
        ApplyVeil(0.0f);
        ApplyContentAlpha(1.0f);
    }

    void GameOverScreenUi::SetSelection(const int index)
    {
        selection_ = index;
        if (const auto retry = retryButton_.get())
            retry->SetHighlighted(selection_ == RETRY_INDEX);
        if (const auto title = titleButton_.get())
            title->SetHighlighted(selection_ == TITLE_INDEX);
    }

    void GameOverScreenUi::BeginCurtain()
    {
        if (phase_ == Phase::Hidden || phase_ == Phase::Closing || phase_ == Phase::Closed)
            return;

        const auto veil = veil_.get();
        curtainFromVeil_ = veil ? static_cast<float>(veil->GetBlendRate()) : 0.0f;
        curtainElapsedSecs_ = 0.0f;
        phase_ = Phase::Closing;
    }

    void GameOverScreenUi::OpenCurtain()
    {
        if (phase_ != Phase::Closed)
            return;

        curtainElapsedSecs_ = 0.0f;
        phase_ = Phase::Opening;
    }

    void GameOverScreenUi::HideImmediately()
    {
        phase_ = Phase::Hidden;
        SetVisualEnabled(false);
    }

    std::shared_ptr<GameOverButton> GameOverScreenUi::ChoiceButton(const int index) const
    {
        return index == RETRY_INDEX ? retryButton_.get() : titleButton_.get();
    }

    void GameOverScreenUi::OnUpdate()
    {
        const float deltaSecs = TickWallClockSeconds();
        if (phase_ == Phase::Hidden)
            return;

        elapsedSecs_ += deltaSecs;
        TickButtons(deltaSecs);

        if (phase_ == Phase::Intro || phase_ == Phase::Waiting)
        {
            UpdateIntro();
            return;
        }

        UpdateCurtain(deltaSecs);
    }

    float GameOverScreenUi::TickWallClockSeconds()
    {
        const int nowMs = GetNowCount();
        const float deltaSecs = static_cast<float>(nowMs - lastTickMs_) / 1000.0f;
        lastTickMs_ = nowMs;

        // GetNowCount は int なのでいつか折り返す。シーン破棄のような重いフレームで一気に進みすぎないよう上限も掛ける
        return std::clamp(deltaSecs, 0.0f, 0.25f);
    }

    void GameOverScreenUi::UpdateIntro()
    {
        ApplyVeil(static_cast<float>(veilBlendRate_) * GameOverSmoothstep(GameOverRate(elapsedSecs_, veilFadeSecs_)));

        if (!isStingPlayed_ && elapsedSecs_ >= stingDelaySecs_)
        {
            isStingPlayed_ = true;
            PlaySe(stingSound_.get());
        }

        if (!isSlabLanded_ && elapsedSecs_ >= slabDelaySecs_ + slabRiseSecs_)
        {
            isSlabLanded_ = true;
            PlaySe(slabLandSound_.get());
        }

        ApplyContentAlpha(1.0f);

        const float readySecs = buttonsDelaySecs_ + buttonStaggerSecs_ + buttonsRiseSecs_ + inputGuardSecs_;
        if (phase_ == Phase::Intro && elapsedSecs_ >= readySecs)
            phase_ = Phase::Waiting;
    }

    void GameOverScreenUi::UpdateCurtain(const float deltaSecs)
    {
        curtainElapsedSecs_ += deltaSecs;

        if (phase_ == Phase::Closing)
        {
            const float closeRate = GameOverRate(curtainElapsedSecs_, curtainCloseSecs_);
            ApplyVeil(std::lerp(curtainFromVeil_, 255.0f, closeRate));
            ApplyContentAlpha(1.0f - closeRate);
            if (closeRate >= 1.0f)
                phase_ = Phase::Closed;
            return;
        }

        if (phase_ == Phase::Closed)
        {
            ApplyVeil(255.0f);
            ApplyContentAlpha(0.0f);
            return;
        }

        const float openRate = GameOverRate(curtainElapsedSecs_, curtainOpenSecs_);
        ApplyVeil(255.0f * (1.0f - openRate));
        if (openRate >= 1.0f)
            HideImmediately();
    }

    void GameOverScreenUi::TickButtons(const float deltaSecs) const
    {
        if (const auto retry = retryButton_.get())
            retry->Tick(deltaSecs);
        if (const auto title = titleButton_.get())
            title->Tick(deltaSecs);
    }

    void GameOverScreenUi::ApplyContentAlpha(const float appearRate) const
    {
        const float slabRate = GameOverRate(elapsedSecs_ - slabDelaySecs_, slabRiseSecs_);
        SetSlabOffset(GameOverSlabOffset(slabRate, slabRiseDistance_px_, slabOvershoot_px_));
        if (const auto slab = slab_.get())
            slab->SetBlendRate(GameOverBlend(std::min(1.0f, slabRate * 3.0f) * appearRate));

        const float dirtRate = GameOverRate(elapsedSecs_ - slabDelaySecs_ - slabRiseSecs_, dirtFadeSecs_);
        if (const auto dirt = slabDirt_.get())
            dirt->SetBlendRate(GameOverBlend(dirtRate * appearRate));

        const std::shared_ptr<GameOverButton> buttons[] = { retryButton_.get(), titleButton_.get() };
        const glm::vec3 basePositions[] = { retryBasePos_, titleBasePos_ };
        for (int i = 0; i < 2; ++i)
        {
            if (!buttons[i])
                continue;

            const float rate = GameOverRate(elapsedSecs_ - buttonsDelaySecs_ - buttonStaggerSecs_ * static_cast<float>(i), buttonsRiseSecs_);
            SetButtonOffset(buttons[i], basePositions[i], buttonRiseDistance_px_ * (1.0f - GameOverEaseOutCubic(rate)));
            buttons[i]->SetAppearRate(rate * appearRate);
        }

        const float hintRate = GameOverRate(elapsedSecs_ - buttonsDelaySecs_ - buttonStaggerSecs_ * 2.0f, buttonsRiseSecs_);
        const int hintBlend = GameOverBlend(hintRate * appearRate);
        if (const auto tag = moveHintTag_.get())
            tag->SetBlendRate(hintBlend);
        if (const auto text = moveHintText_.get())
            text->SetBlendRate(hintBlend);
        if (const auto tag = confirmHintTag_.get())
            tag->SetBlendRate(hintBlend);
        if (const auto text = confirmHintText_.get())
            text->SetBlendRate(hintBlend);
    }

    void GameOverScreenUi::ApplyVeil(const float blendRate) const
    {
        if (const auto veil = veil_.get())
            veil->SetBlendRate(std::clamp(static_cast<int>(blendRate), 0, 255));
    }

    void GameOverScreenUi::SetSlabOffset(const float offsetY) const
    {
        const auto slab = slab_.get();
        if (!slab)
            return;

        slab->Transform().SetLocalPos(slabBasePos_ + glm::vec3(0.0f, offsetY, 0.0f));
    }

    void GameOverScreenUi::SetButtonOffset(
        const std::shared_ptr<GameOverButton>& button,
        const glm::vec3& basePos,
        const float offsetY) const
    {
        button->Transform().SetLocalPos(basePos + glm::vec3(0.0f, offsetY, 0.0f));
    }

    void GameOverScreenUi::SetVisualEnabled(const bool isEnabled) const
    {
        if (const auto visualRoot = visualRoot_.get())
            visualRoot->SetEnable(isEnabled);
    }

    void GameOverScreenUi::PlaySe(const std::shared_ptr<Asset::SoundFile>& sound) const
    {
        if (!sound)
            return;

        const int handle = sound->GetDxLibHandle();
        if (handle == -1)
            return;

        PlaySoundMem(handle, DX_PLAYTYPE_BACK, TRUE);
    }

    void GameOverScreenUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("visualRoot_", visualRoot_);
        ImGuiHelper::OnDrawInputField("veil_", veil_);
        ImGuiHelper::OnDrawInputField("slab_", slab_);
        ImGuiHelper::OnDrawInputField("slabDirt_", slabDirt_);
        ImGuiHelper::OnDrawInputField("retryButton_", retryButton_);
        ImGuiHelper::OnDrawInputField("titleButton_", titleButton_);
        ImGuiHelper::OnDrawInputField("moveHintTag_", moveHintTag_);
        ImGuiHelper::OnDrawInputField("moveHintText_", moveHintText_);
        ImGuiHelper::OnDrawInputField("confirmHintTag_", confirmHintTag_);
        ImGuiHelper::OnDrawInputField("confirmHintText_", confirmHintText_);
        ImGuiHelper::OnDrawInputField("stingSound_", stingSound_);
        ImGuiHelper::OnDrawInputField("slabLandSound_", slabLandSound_);
        ImGuiHelper::OnDrawInputField("veilBlendRate_", veilBlendRate_);
        ImGuiHelper::OnDrawInputField("veilFadeSecs_", veilFadeSecs_);
        ImGuiHelper::OnDrawInputField("stingDelaySecs_", stingDelaySecs_);
        ImGuiHelper::OnDrawInputField("slabDelaySecs_", slabDelaySecs_);
        ImGuiHelper::OnDrawInputField("slabRiseSecs_", slabRiseSecs_);
        ImGuiHelper::OnDrawInputField("slabRiseDistance_px_", slabRiseDistance_px_);
        ImGuiHelper::OnDrawInputField("slabOvershoot_px_", slabOvershoot_px_);
        ImGuiHelper::OnDrawInputField("dirtFadeSecs_", dirtFadeSecs_);
        ImGuiHelper::OnDrawInputField("buttonsDelaySecs_", buttonsDelaySecs_);
        ImGuiHelper::OnDrawInputField("buttonsRiseSecs_", buttonsRiseSecs_);
        ImGuiHelper::OnDrawInputField("buttonStaggerSecs_", buttonStaggerSecs_);
        ImGuiHelper::OnDrawInputField("buttonRiseDistance_px_", buttonRiseDistance_px_);
        ImGuiHelper::OnDrawInputField("inputGuardSecs_", inputGuardSecs_);
        ImGuiHelper::OnDrawInputField("curtainCloseSecs_", curtainCloseSecs_);
        ImGuiHelper::OnDrawInputField("curtainOpenSecs_", curtainOpenSecs_);

        if (ImGui::Button("Show (preview)"))
            Show();
        ImGui::SameLine();
        if (ImGui::Button("Hide"))
            HideImmediately();
        ImGui::Text("phase: %d  elapsed: %.2f", static_cast<int>(phase_), elapsedSecs_);
    }
}

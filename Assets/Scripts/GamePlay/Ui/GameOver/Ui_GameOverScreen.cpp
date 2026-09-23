#include "Ui_GameOverScreen.h"

#include <algorithm>

#include "DxLib.h"
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

        // 石版が浮き上がり切るまでの割合。残りで落ちて着地する
        constexpr float GAME_OVER_SLAB_PEAK_RATE = 0.65f;

        /** @brief delaySecs だけ 0 のまま待ってから、durationSecs で 1 まで一定の速さで上がる */
        tweeny::tween<float> GameOverDelayedRate(const float delaySecs, const float durationSecs)
        {
            return tweeny::from(0.0f).to(0.0f).during(Ms(delaySecs))
                .to(1.0f).during(Ms(durationSecs));
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
        PlayIntroTweens();
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
        const float fromVeil = veil ? static_cast<float>(veil->GetBlendRate()) : 0.0f;
        veilTween_.Play(tweeny::from(fromVeil).to(255.0f).during(Ms(curtainCloseSecs_)));
        phase_ = Phase::Closing;
    }

    void GameOverScreenUi::OpenCurtain()
    {
        if (phase_ != Phase::Closed)
            return;

        veilTween_.Play(tweeny::from(255.0f).to(0.0f).during(Ms(curtainOpenSecs_)));
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
        TickContentTweens(deltaSecs);
        TickButtons(deltaSecs);

        if (phase_ == Phase::Intro || phase_ == Phase::Waiting)
        {
            UpdateIntro(deltaSecs);
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

    void GameOverScreenUi::PlayIntroTweens()
    {
        veilTween_.Play(tweeny::from(0.0f).to(static_cast<float>(veilBlendRate_))
            .during(Ms(veilFadeSecs_)).via(Ease(EaseType::SmoothStep)));

        // 下から勢いよく持ち上がって少し浮き、加速しながら落ちて止まる
        slabOffsetTween_.Play(tweeny::from(slabRiseDistance_px_).to(slabRiseDistance_px_).during(Ms(slabDelaySecs_))
            .to(-slabOvershoot_px_).during(Ms(slabRiseSecs_ * GAME_OVER_SLAB_PEAK_RATE)).via(Ease(EaseType::OutCubic))
            .to(0.0f).during(Ms(slabRiseSecs_ * (1.0f - GAME_OVER_SLAB_PEAK_RATE))).via(Ease(EaseType::InQuad)));
        slabAlphaTween_.Play(GameOverDelayedRate(slabDelaySecs_, slabRiseSecs_ / 3.0f));
        dirtAlphaTween_.Play(GameOverDelayedRate(slabDelaySecs_ + slabRiseSecs_, dirtFadeSecs_));

        for (size_t i = 0; i < buttonRiseTweens_.size(); ++i)
            buttonRiseTweens_[i].Play(GameOverDelayedRate(buttonsDelaySecs_ + buttonStaggerSecs_ * static_cast<float>(i), buttonsRiseSecs_));
        hintAlphaTween_.Play(GameOverDelayedRate(buttonsDelaySecs_ + buttonStaggerSecs_ * 2.0f, buttonsRiseSecs_));
    }

    void GameOverScreenUi::TickContentTweens(const float deltaSecs)
    {
        slabOffsetTween_.Tick(deltaSecs);
        slabAlphaTween_.Tick(deltaSecs);
        dirtAlphaTween_.Tick(deltaSecs);
        for (auto& tween : buttonRiseTweens_)
            tween.Tick(deltaSecs);
        hintAlphaTween_.Tick(deltaSecs);
    }

    void GameOverScreenUi::UpdateIntro(const float deltaSecs)
    {
        veilTween_.Tick(deltaSecs);
        ApplyVeil(veilTween_.Value());

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
        if (phase_ == Phase::Closing)
        {
            const bool isFinished = veilTween_.Tick(deltaSecs);
            ApplyVeil(veilTween_.Value());
            ApplyContentAlpha(1.0f - veilTween_.Progress());
            if (isFinished)
                phase_ = Phase::Closed;
            return;
        }

        if (phase_ == Phase::Closed)
        {
            ApplyVeil(255.0f);
            ApplyContentAlpha(0.0f);
            return;
        }

        const bool isFinished = veilTween_.Tick(deltaSecs);
        ApplyVeil(veilTween_.Value());
        if (isFinished)
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
        SetSlabOffset(slabOffsetTween_.Value());
        if (const auto slab = slab_.get())
            slab->SetBlendRate(GameOverBlend(slabAlphaTween_.Value() * appearRate));

        if (const auto dirt = slabDirt_.get())
            dirt->SetBlendRate(GameOverBlend(dirtAlphaTween_.Value() * appearRate));

        const std::shared_ptr<GameOverButton> buttons[] = { retryButton_.get(), titleButton_.get() };
        const glm::vec3 basePositions[] = { retryBasePos_, titleBasePos_ };
        for (int i = 0; i < 2; ++i)
        {
            if (!buttons[i])
                continue;

            const float rate = buttonRiseTweens_[static_cast<size_t>(i)].Value();
            SetButtonOffset(buttons[i], basePositions[i], buttonRiseDistance_px_ * (1.0f - Ease(EaseType::OutCubic).Ease(rate)));
            buttons[i]->SetAppearRate(rate * appearRate);
        }

        const int hintBlend = GameOverBlend(hintAlphaTween_.Value() * appearRate);
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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameOverScreenUi);
#pragma endregion

#include "Ui_LoadingScreen.h"

#include <algorithm>
#include <cmath>

#include "DxLib.h"
#include "../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"

using GameCore::Scene::Main::SceneLoadStep;

namespace
{
    /** @brief 段階ごとの進捗の取り分。合計 1.0 */
    float LoadingScreenStepWeight(const SceneLoadStep step)
    {
        switch (step)
        {
        case SceneLoadStep::Deserializing: return 0.55f;
        case SceneLoadStep::Warmup:        return 0.20f;
        case SceneLoadStep::Connecting:    return 0.20f;
        case SceneLoadStep::Spawning:      return 0.05f;
        default:                           return 0.0f;
        }
    }

    /** @brief その段階より前に積み上がっている取り分 */
    float LoadingScreenWeightBefore(const SceneLoadStep step)
    {
        float sum = 0.0f;
        for (const SceneLoadStep passed : {SceneLoadStep::Deserializing, SceneLoadStep::Warmup,
                                           SceneLoadStep::Connecting, SceneLoadStep::Spawning})
        {
            if (passed == step)
                return sum;

            sum += LoadingScreenStepWeight(passed);
        }
        return sum;
    }

    /**
     * @brief 残り時間が原理的に分からない段階を埋める飽和カーブ。
     *        必ず単調増加しつつ 1.0 には届かないので、実完了時の詰めと噛み合う
     */
    float LoadingScreenSaturate(const float elapsedSecs, const float timeConstantSecs)
    {
        return 1.0f - std::exp(-elapsedSecs / timeConstantSecs);
    }

    float LoadingScreenStepRatio(const SceneLoadStep step, const float elapsedSecs, const float deserializeProgress01)
    {
        switch (step)
        {
        case SceneLoadStep::Deserializing: return deserializeProgress01;
        case SceneLoadStep::Warmup:        return LoadingScreenSaturate(elapsedSecs, 0.35f);
        case SceneLoadStep::Connecting:    return LoadingScreenSaturate(elapsedSecs, 1.20f) * 0.95f;
        case SceneLoadStep::Spawning:      return LoadingScreenSaturate(elapsedSecs, 0.25f);
        default:                           return 0.0f;
        }
    }
}

namespace GamePlay::Ui
{
    void LoadingScreenUi::Show(const std::shared_ptr<Asset::StageData>& stageData)
    {
        if (stageData)
        {
            ApplyStageData(*stageData);
            shownStageData_ = stageData;
        }

        if (const auto hint = hintCard_.get())
            hint->Reset();

        phase_ = Phase::FadingIn;
        step_ = SceneLoadStep::Deserializing;
        statusMessage_.clear();
        stepElapsedSecs_ = 0.0f;
        shownElapsedSecs_ = 0.0f;
        displayedProgress_ = 0.0f;
        lastShownPercent_ = -1;
        isHideRequested_ = false;
        lastTickMs_ = GetNowCount();

        SetVisualEnabled(true);
        ApplyCoverBlendRate();
    }

    void LoadingScreenUi::SetStep(const SceneLoadStep step)
    {
        if (step_ == step)
            return;

        step_ = step;
        stepElapsedSecs_ = 0.0f;
    }

    void LoadingScreenUi::Fail(const std::string& message)
    {
        step_ = SceneLoadStep::Failed;
        stepElapsedSecs_ = 0.0f;
        statusMessage_ = message;
    }

    void LoadingScreenUi::BeginHide()
    {
        isHideRequested_ = true;
    }

    Coroutine::Task<void> LoadingScreenUi::WaitCoverOpaqueAsync() const
    {
        co_await Coroutine::WaitUntil([this] { return IsCoverOpaque(); });
    }

    void LoadingScreenUi::OnStart()
    {
        // 起動直後から出ていないように、常駐しているぶんを自分で畳んでおく
        SetVisualEnabled(false);
        coverBlendRate_ = 0.0f;
        ApplyCoverBlendRate();
        lastTickMs_ = GetNowCount();
    }

    void LoadingScreenUi::OnUpdate()
    {
        const float deltaSecs = TickWallClockSeconds();
        if (phase_ == Phase::Hidden)
            return;

        stepElapsedSecs_ += deltaSecs;
        shownElapsedSecs_ += deltaSecs;

        UpdateProgress(deltaSecs);
        UpdateCoverFade(deltaSecs);
    }

    float LoadingScreenUi::TickWallClockSeconds()
    {
        const int nowMs = GetNowCount();
        const float deltaSecs = static_cast<float>(nowMs - lastTickMs_) / 1000.0f;
        lastTickMs_ = nowMs;

        // GetNowCount は int なのでいつか折り返す。旧シーン破棄のような重いフレームで
        // 一気に進みすぎないよう上限も掛ける
        return std::clamp(deltaSecs, 0.0f, 0.25f);
    }

    void LoadingScreenUi::ApplyStageData(const Asset::StageData& stageData) const
    {
        if (const auto thumbnail = stageData.ThumbnailSprite())
        {
            if (const auto cardThumbnail = cardThumbnail_.get())
                cardThumbnail->SetSprite(thumbnail);
            if (const auto backdrop = backdrop_.get())
                backdrop->SetSprite(thumbnail);
        }

        if (const auto element = stageData.ElementSprite())
        {
            if (const auto cardElement = cardElement_.get())
                cardElement->SetSprite(element);
        }

        if (const auto stageName = stageNameText_.get())
            stageName->SetText(stageData.DisplayName());
        if (const auto tag = tagText_.get())
            tag->SetText(stageData.TagText());
        if (const auto pips = difficultyPips_.get())
            pips->SetDifficulty(stageData.Difficulty());
    }

    void LoadingScreenUi::UpdateCoverFade(const float deltaSecs)
    {
        if (phase_ == Phase::FadingIn)
        {
            coverBlendRate_ += 255.0f * deltaSecs / std::max(fadeInSecs_, 0.01f);
            if (coverBlendRate_ >= 255.0f)
            {
                coverBlendRate_ = 255.0f;
                phase_ = Phase::Visible;
            }
        }
        else if (phase_ == Phase::Visible)
        {
            if (CanHide())
                phase_ = Phase::FadingOut;
        }
        else if (phase_ == Phase::FadingOut)
        {
            coverBlendRate_ -= 255.0f * deltaSecs / std::max(fadeOutSecs_, 0.01f);
            if (coverBlendRate_ <= 0.0f)
            {
                coverBlendRate_ = 0.0f;
                phase_ = Phase::Hidden;
                SetVisualEnabled(false);
            }
        }

        ApplyCoverBlendRate();
    }

    void LoadingScreenUi::UpdateProgress(const float deltaSecs)
    {
        const float rawTarget = CalcRawProgress();
        const float follow = 1.0f - std::exp(-std::max(progressFollowRate_, 0.01f) * deltaSecs);

        // 表示は決して後戻りさせない。段階が切り替わって目標が一時的に下がっても据え置く
        const float next = displayedProgress_ + (rawTarget - displayedProgress_) * follow;
        displayedProgress_ = std::max(displayedProgress_, next);

        if (step_ == SceneLoadStep::Completed)
            displayedProgress_ = std::min(1.0f, displayedProgress_ + deltaSecs / std::max(finishSecs_, 0.01f));

        if (const auto progressBar = progressBar_.get())
            progressBar->SetValue(displayedProgress_);

        const int percent = static_cast<int>(displayedProgress_ * 100.0f);
        if (percent == lastShownPercent_)
            return;

        // TextRenderer は SetText のたびにテクスチャを作り直すので、整数%が動いた時だけ触る
        lastShownPercent_ = percent;
        if (const auto statusText = statusText_.get())
        {
            statusText->SetText(step_ == SceneLoadStep::Failed
                ? statusMessage_
                : "転 送 中 …   " + std::to_string(percent) + "%");
        }
    }

    void LoadingScreenUi::SetVisualEnabled(const bool isEnabled) const
    {
        if (const auto visualRoot = visualRoot_.get())
            visualRoot->SetEnable(isEnabled);
    }

    void LoadingScreenUi::ApplyCoverBlendRate() const
    {
        const int blendRate = static_cast<int>(coverBlendRate_);
        if (const auto cover = cover_.get())
            cover->SetBlendRate(blendRate);
        if (const auto backdrop = backdrop_.get())
            backdrop->SetBlendRate(blendRate * backdropBlendRate_ / 255);
    }

    float LoadingScreenUi::CalcRawProgress() const
    {
        if (step_ == SceneLoadStep::Completed)
            return 1.0f;
        if (step_ == SceneLoadStep::Idle || step_ == SceneLoadStep::Failed)
            return displayedProgress_;

        const float deserializeProgress01 =
            Core::Application::ApplicationBase::GameWindow()->SceneLoadProgress01();

        return LoadingScreenWeightBefore(step_)
             + LoadingScreenStepWeight(step_) * LoadingScreenStepRatio(step_, stepElapsedSecs_, deserializeProgress01);
    }

    bool LoadingScreenUi::CanHide() const
    {
        return isHideRequested_
            && shownElapsedSecs_ >= minShowSecs_
            && displayedProgress_ >= 1.0f;
    }

    void LoadingScreenUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("visualRoot_", visualRoot_);
        ImGuiHelper::OnDrawInputField("cover_", cover_);
        ImGuiHelper::OnDrawInputField("backdrop_", backdrop_);
        ImGuiHelper::OnDrawInputField("cardThumbnail_", cardThumbnail_);
        ImGuiHelper::OnDrawInputField("cardElement_", cardElement_);
        ImGuiHelper::OnDrawInputField("stageNameText_", stageNameText_);
        ImGuiHelper::OnDrawInputField("tagText_", tagText_);
        ImGuiHelper::OnDrawInputField("difficultyPips_", difficultyPips_);
        ImGuiHelper::OnDrawInputField("progressBar_", progressBar_);
        ImGuiHelper::OnDrawInputField("statusText_", statusText_);
        ImGuiHelper::OnDrawInputField("hintCard_", hintCard_);
        ImGuiHelper::OnDrawInputField("fadeInSecs_", fadeInSecs_);
        ImGuiHelper::OnDrawInputField("fadeOutSecs_", fadeOutSecs_);
        ImGuiHelper::OnDrawInputField("minShowSecs_", minShowSecs_);
        ImGuiHelper::OnDrawInputField("progressFollowRate_", progressFollowRate_);
        ImGuiHelper::OnDrawInputField("finishSecs_", finishSecs_);
        ImGuiHelper::OnDrawInputField("backdropBlendRate_", backdropBlendRate_);
        ImGui::Text("progress: %.3f", displayedProgress_);
        ImGui::Text("step: %d", static_cast<int>(step_));
    }
}

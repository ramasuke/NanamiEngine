#include "Ui_LoadingScreen.h"

#include <algorithm>
#include <cmath>

#include "DxLib.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../Sound/UiSoundBank.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

using GameCore::Scene::Main::SceneLoadStep;

namespace
{
    /**
     * @brief 段階ごとの進捗の取り分。合計 1.0。
     *        接続を挟まないシーンでは Connecting の取り分を読み込みと暖機へ回す
     */
    float LoadingScreenStepWeight(const SceneLoadStep step, const bool hasNetworkStep)
    {
        switch (step)
        {
        case SceneLoadStep::Deserializing: return hasNetworkStep ? 0.55f : 0.65f;
        case SceneLoadStep::Warmup:        return hasNetworkStep ? 0.20f : 0.30f;
        case SceneLoadStep::Connecting:    return hasNetworkStep ? 0.20f : 0.0f;
        case SceneLoadStep::Spawning:      return 0.05f;
        default:                           return 0.0f;
        }
    }

    /** @brief その段階より前に積み上がっている取り分 */
    float LoadingScreenWeightBefore(const SceneLoadStep step, const bool hasNetworkStep)
    {
        float sum = 0.0f;
        for (const SceneLoadStep passed : {SceneLoadStep::Deserializing, SceneLoadStep::Warmup,
                                           SceneLoadStep::Connecting, SceneLoadStep::Spawning})
        {
            if (passed == step)
                return sum;

            sum += LoadingScreenStepWeight(passed, hasNetworkStep);
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
    void LoadingScreenUi::Show(
        const std::optional<GameCore::Scene::Main::SceneType> from,
        const GameCore::Scene::Main::SceneType to,
        const GameCore::Scene::Main::SceneTransitionOptions& options)
    {
        const auto route = FindRoute(from, to);
        if (!route)
        {
            NanamiEngine::Module::LogWarning(
                "LoadingScreenUi: " + std::string(GameCore::Scene::Main::ToString(to)) + " への航路が routes_ にありません");
        }

        routeStatusText_ = route ? route->StatusText() : std::string("移動中…");
        hasNetworkStep_  = route && route->HasNetworkStep();
        if (const auto routeMap = routeMap_.get(); routeMap && route)
            routeMap->Begin(*route, options.isStageCleared);

        if (const auto hint = hintCard_.get())
            hint->Reset();

        step_ = SceneLoadStep::Deserializing;
        statusMessage_.clear();
        stepElapsedSecs_ = 0.0f;
        shownElapsedSecs_ = 0.0f;
        displayedProgress_ = 0.0f;
        lastShownPercent_ = -1;
        isHideRequested_ = false;
        isStatusDirty_ = true;

        switch (phase_)
        {
        case Phase::Hidden:
            lastTickMs_ = GetNowCount();
            PlayCover(0.0f, 255.0f, fadeInSecs_);
            phase_ = Phase::CoveringGame;
            break;
        case Phase::RevealingGame:
            // 地図は消えたあと。幕が明け切る前なので、その濃さから覆い直す
            PlayCover(coverTween_.Value(), 255.0f, fadeInSecs_);
            phase_ = Phase::CoveringGame;
            break;
        case Phase::CoveringMap:
            // 地図はまだ出ている。幕を明けて見せ直す
            PlayCover(coverTween_.Value(), 0.0f, fadeInSecs_);
            phase_ = Phase::RevealingMap;
            break;
        case Phase::CoveringGame:
        case Phase::RevealingMap:
        case Phase::Visible:
            break;
        }

        ApplyCoverBlendRate();
        UpdateStatusText();

        if (const auto bgm = bgm_.get(); bgm && CheckSoundMem(bgm->GetDxLibHandle()) != 1)
        {
            ChangeVolumeSoundMem(0, bgm->GetDxLibHandle());
            PlaySoundMem(bgm->GetDxLibHandle(), DX_PLAYTYPE_LOOP, TRUE);
        }
        UpdateBgm();
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
        isStatusDirty_ = true;
        UpdateStatusText();
    }

    void LoadingScreenUi::BeginHide()
    {
        isHideRequested_ = true;
    }

    bool LoadingScreenUi::IsCoverOpaque() const
    {
        switch (phase_)
        {
        case Phase::RevealingMap:
        case Phase::Visible:
        case Phase::CoveringMap:
            return true;
        case Phase::CoveringGame:
        case Phase::RevealingGame:
            return coverTween_.Value() >= 255.0f;
        case Phase::Hidden:
            return false;
        }
        return false;
    }

    Coroutine::Task<void> LoadingScreenUi::WaitCoverOpaqueAsync() const
    {
        co_await Coroutine::WaitUntil([this] { return IsCoverOpaque(); });
    }

    void LoadingScreenUi::OnStart()
    {
        // 起動直後から出ていないように、常駐しているぶんを自分で畳んでおく。
        // 起動時のタイトルの読み込みは OnStart より先に Show するので、そのときは触らない
        lastTickMs_ = GetNowCount();
        if (phase_ != Phase::Hidden)
            return;

        SetVisualEnabled(false);
        ApplyCoverBlendRate();
    }

    void LoadingScreenUi::OnUpdate()
    {
        const float deltaSecs = TickWallClockSeconds();
        if (phase_ == Phase::Hidden)
            return;

        stepElapsedSecs_ += deltaSecs;
        shownElapsedSecs_ += deltaSecs;
        animationSecs_ += deltaSecs;

        UpdateProgress(deltaSecs);
        UpdateCoverFade(deltaSecs);

        if (const auto routeMap = routeMap_.get())
            routeMap->Tick(displayedProgress_, animationSecs_, deltaSecs);
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

    std::shared_ptr<Asset::LoadingRouteData> LoadingScreenUi::FindRoute(
        const std::optional<GameCore::Scene::Main::SceneType> from,
        const GameCore::Scene::Main::SceneType to) const
    {
        std::shared_ptr<Asset::LoadingRouteData> best;
        int bestScore = 0;
        for (const auto& field : routes_)
        {
            const auto route = field.get();
            if (!route || !route->Matches(from.value_or(to), from.has_value(), to))
                continue;

            if (route->MatchScore() > bestScore)
            {
                best = route;
                bestScore = route->MatchScore();
            }
        }
        return best;
    }

    void LoadingScreenUi::UpdateCoverFade(const float deltaSecs)
    {
        const bool isCoverFinished = coverTween_.Tick(deltaSecs);

        switch (phase_)
        {
        case Phase::CoveringGame:
            if (isCoverFinished)
            {
                SetVisualEnabled(true);
                phase_ = Phase::RevealingMap;
                PlayCover(255.0f, 0.0f, fadeInSecs_);
            }
            break;
        case Phase::RevealingMap:
            if (isCoverFinished)
                phase_ = Phase::Visible;
            break;
        case Phase::Visible:
            if (CanHide())
            {
                Sound::UiSoundBank::Play(Sound::UiSe::LoadingDone);
                phase_ = Phase::CoveringMap;
                PlayCover(0.0f, 255.0f, fadeOutSecs_);
            }
            break;
        case Phase::CoveringMap:
            if (isCoverFinished)
            {
                SetVisualEnabled(false);
                phase_ = Phase::RevealingGame;
                PlayCover(255.0f, 0.0f, fadeOutSecs_);
            }
            break;
        case Phase::RevealingGame:
            if (isCoverFinished)
                phase_ = Phase::Hidden;
            break;
        case Phase::Hidden:
            break;
        }

        ApplyCoverBlendRate();
        UpdateBgm();
    }

    void LoadingScreenUi::PlayCover(const float from, const float to, const float fullFadeSecs)
    {
        const float secs = std::max(fullFadeSecs, 0.01f) * std::abs(to - from) / 255.0f;
        coverTween_.Play(tweeny::from(from).to(to).during(LibCore::Tween::Ms(secs)));
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

        UpdateStatusText();
    }

    void LoadingScreenUi::UpdateStatusText()
    {
        const bool isFailed = step_ == SceneLoadStep::Failed;
        if (isStatusDirty_)
        {
            isStatusDirty_ = false;
            if (const auto statusText = statusText_.get())
                statusText->SetText(isFailed ? statusMessage_ : routeStatusText_);
            if (const auto percentText = percentText_.get())
                percentText->SetEnable(isVisualEnabled_ && !isFailed);
        }

        if (isFailed)
            return;

        const int percent = static_cast<int>(displayedProgress_ * 100.0f);
        if (percent == lastShownPercent_)
            return;

        // TextRenderer は SetText のたびにテクスチャを作り直すので、整数%が動いた時だけ触る
        lastShownPercent_ = percent;
        if (const auto percentText = percentText_.get())
            percentText->SetText(std::to_string(percent) + "%");
    }

    void LoadingScreenUi::SetVisualEnabled(const bool isEnabled)
    {
        isVisualEnabled_ = isEnabled;
        if (const auto visualRoot = visualRoot_.get())
            visualRoot->SetEnable(isEnabled);

        // 親の SetEnable で一律に書き換わった子の出し分けを付け直す
        if (const auto routeMap = routeMap_.get())
            routeMap->SetShown(isEnabled);
        if (const auto percentText = percentText_.get())
            percentText->SetEnable(isEnabled && step_ != SceneLoadStep::Failed);
    }

    void LoadingScreenUi::ApplyCoverBlendRate() const
    {
        const auto cover = cover_.get();
        if (!cover)
            return;

        const int blendRate = static_cast<int>(coverTween_.Value());
        cover->SetEnable(blendRate > 0);
        cover->SetBlendRate(blendRate);
    }

    void LoadingScreenUi::UpdateBgm() const
    {
        const auto bgm = bgm_.get();
        if (!bgm)
            return;

        const float cover01 = coverTween_.Value() / 255.0f;
        float volume01 = 0.0f;
        switch (phase_)
        {
        case Phase::CoveringGame:
            volume01 = cover01;
            break;
        case Phase::RevealingMap:
        case Phase::Visible:
            volume01 = 1.0f;
            break;
        case Phase::CoveringMap:
            volume01 = 1.0f - cover01;
            break;
        case Phase::RevealingGame:
        case Phase::Hidden:
            // NOTE: 地図を消した時点で次のシーンの BGM に明け渡す
            StopSoundMem(bgm->GetDxLibHandle());
            return;
        }

        ChangeVolumeSoundMem(static_cast<int>(static_cast<float>(bgmVolume_) * std::clamp(volume01, 0.0f, 1.0f)),
                             bgm->GetDxLibHandle());
    }

    float LoadingScreenUi::CalcRawProgress() const
    {
        if (step_ == SceneLoadStep::Completed)
            return 1.0f;
        if (step_ == SceneLoadStep::Idle || step_ == SceneLoadStep::Failed)
            return displayedProgress_;

        const float deserializeProgress01 =
            Core::Application::ApplicationBase::GameWindow()->SceneLoadProgress01();

        return LoadingScreenWeightBefore(step_, hasNetworkStep_)
             + LoadingScreenStepWeight(step_, hasNetworkStep_)
             * LoadingScreenStepRatio(step_, stepElapsedSecs_, deserializeProgress01);
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
        ImGuiHelper::OnDrawInputField("routeMap_", routeMap_);
        ImGuiHelper::OnDrawInputField("routes_", routes_, [this]
        {
            if (ImGui::Button("Add##routes_"))
                routes_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("statusText_", statusText_);
        ImGuiHelper::OnDrawInputField("percentText_", percentText_);
        ImGuiHelper::OnDrawInputField("hintCard_", hintCard_);
        ImGuiHelper::OnDrawInputField("fadeInSecs_", fadeInSecs_);
        ImGuiHelper::OnDrawInputField("fadeOutSecs_", fadeOutSecs_);
        ImGuiHelper::OnDrawInputField("minShowSecs_", minShowSecs_);
        ImGuiHelper::OnDrawInputField("progressFollowRate_", progressFollowRate_);
        ImGuiHelper::OnDrawInputField("finishSecs_", finishSecs_);
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGui::SliderInt("bgmVolume_", &bgmVolume_, 0, 255);
        ImGui::Text("progress: %.3f", displayedProgress_);
        ImGui::Text("step: %d  phase: %d", static_cast<int>(step_), static_cast<int>(phase_));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LoadingScreenUi);
#pragma endregion

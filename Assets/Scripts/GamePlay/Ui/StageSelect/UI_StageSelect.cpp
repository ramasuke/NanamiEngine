#include "UI_StageSelect.h"

#include "../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace GamePlay::Ui
{
    void StageSelectUi::OnStart()
    {
        Coroutine::StartCoroutine(StartStageSelectAsync());
    }

    void StageSelectUi::OnDestroy()
    {
        ComponentBase::OnDestroy();
    }

    std::vector<std::weak_ptr<StageSelectStageUi>> StageSelectUi::Stages() const
    {
        std::vector<std::weak_ptr<StageSelectStageUi>> result;
        result.reserve(stageSelectButtons_.size());
        for (const auto& stage : stageSelectButtons_)
        {
            result.push_back(stage.get());
        }
        return result;
    }

    void StageSelectUi::HighlightSelectedStage(const size_t selectedIndex)
    {
        for (size_t i = 0; i < stageSelectButtons_.size(); ++i)
        {
            stageSelectButtons_[i]->SetHighlighted(i == selectedIndex);
        }
    }

    void StageSelectUi::SetWorldEnterButtonEnabled(const bool isEnabled)
    {
        const auto& sprite = isEnabled ? worldEnterButtonActiveSprite_ : worldEnterButtonDisabledSprite_;
        if (const auto renderer = worldEnterButton_->Components().Catch<NanamiUi::IInteractivableRenderer>().lock())
        {
            renderer->SetSprite(sprite.get());
        }
        worldEnterButtonGlow_->SetEnable(isEnabled);
    }

    void StageSelectUi::ShowMapMarker(const glm::vec2& position, const bool isCleared)
    {
        if (const auto entity = mapMarker_->Entity().lock())
        {
            entity->SetEnable(true);
        }
        mapMarker_->MoveTo(position);
        mapMarker_->SetCleared(isCleared);
    }

    void StageSelectUi::ShowStageDetail(const Asset::StageData& stage)
    {
        detailPreview_->SetSprite(stage.ThumbnailSprite());
        detailPreview_->SetEnable(true);
        detailElement_->SetSprite(stage.ElementSprite());
        detailElement_->SetEnable(true);
        detailLabel_->SetEnable(true);
        detailTitle_->SetText(stage.DisplayName());
        detailTag_->SetText(stage.TagText());
        detailDifficulty_->SetDifficulty(stage.Difficulty());
        SetDetailDifficultyVisible(true);

        const auto& lines = stage.DescriptionLines();
        for (size_t i = 0; i < detailDescriptionLines_.size(); ++i)
        {
            detailDescriptionLines_[i]->SetText(i < lines.size() ? lines[i] : "");
        }
    }

    void StageSelectUi::ShowNoSelectionDetail()
    {
        detailPreview_->SetEnable(false);
        detailElement_->SetEnable(false);
        detailLabel_->SetEnable(false);
        detailTitle_->SetText(noSelectionTitle_);
        detailTag_->SetText("");
        SetDetailDifficultyVisible(false);

        for (const auto& line : detailDescriptionLines_)
        {
            line->SetText("");
        }
    }

    void StageSelectUi::SetDetailDifficultyVisible(const bool isVisible)
    {
        if (const auto entity = detailDifficulty_->Entity().lock())
        {
            entity->SetEnable(isVisible);
        }
    }

    Coroutine::Task<void> StageSelectUi::StartStageSelectAsync()
    {
        co_await AppearBackGroundMaskAsync();
    }

    Coroutine::Task<void> StageSelectUi::AppearBackGroundMaskAsync()
    {
        Coroutine::StartCoroutine(FadeBlendRateAsync(backGround_.get(), 0, 255));
        co_await FadeBlendRateAsync(backGroundMask_.get(), 0, backGroundMaskBlendRate_);
    }

    Coroutine::Task<void> StageSelectUi::PlayEnterWorldTransitionAsync(
        const GameCore::Scene::Main::SceneType sceneType,
        const std::shared_ptr<Asset::StageData> stageData)
    {
        if (isEnteringWorld_)
            co_return;

        isEnteringWorld_ = true;

        // ロード画面が覆い切ってから遷移を頼む。RequestChangeScene はキューに積むだけなので、
        // MainIsland の Dispose が始まるのはさらに次のフレーム
        auto& loadingScreen = GameCore::Game::Instance().LoadingScreen();
        loadingScreen.Show(stageData);
        co_await loadingScreen.WaitCoverOpaqueAsync();

        GameCore::Game::Instance().Scenes().RequestChangeScene(sceneType);
        isEnteringWorld_ = false;
    }

    Coroutine::Task<void> StageSelectUi::FadeBlendRateAsync(
        const std::weak_ptr<NanamiUi::BlendImageRenderer> renderer, const int from, const int to)
    {
        const int step = to > from ? 1 : -1;
        for (int rate = from; rate != to; rate += step)
        {
            const auto locked = renderer.lock();
            if (!locked)
                co_return;

            locked->SetBlendRate(rate);
            co_await Coroutine::WaitYield();
        }

        if (const auto locked = renderer.lock())
            locked->SetBlendRate(to);
    }

    Coroutine::Task<void> StageSelectUi::FadeBlendRateAsync(
        const std::weak_ptr<NanamiUi::MovieRenderer> renderer, const int from, const int to)
    {
        const int step = to > from ? 1 : -1;
        for (int rate = from; rate != to; rate += step)
        {
            const auto locked = renderer.lock();
            if (!locked)
                co_return;

            locked->SetBlendRate(rate);
            co_await Coroutine::WaitYield();
        }

        if (const auto locked = renderer.lock())
            locked->SetBlendRate(to);
    }

    void StageSelectUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("backGroundMask_", backGroundMask_);
        ImGuiHelper::OnDrawInputField("backGroundMaskBlendRate_", backGroundMaskBlendRate_);
        ImGuiHelper::OnDrawInputField("stageSelectButtons_", stageSelectButtons_, [this]
        {
            if (ImGui::Button("Add"))
            {
                stageSelectButtons_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("stageSelectBackGroundMask_", stageSelectBackGroundMask_);
        ImGuiHelper::OnDrawInputField("stageSelectBackGroundMaskBlendRate_", stageSelectBackGroundMaskBlendRate_);
        ImGuiHelper::OnDrawInputField("worldMovieRenderer_", worldMovieRenderer_);
        ImGuiHelper::OnDrawInputField("worldEnterButton_", worldEnterButton_);
        ImGuiHelper::OnDrawInputField("worldEnterButtonGlow_", worldEnterButtonGlow_);
        ImGuiHelper::OnDrawInputField("backGround_", backGround_);
        ImGuiHelper::OnDrawInputField("worldEnterButtonActiveSprite_", worldEnterButtonActiveSprite_);
        ImGuiHelper::OnDrawInputField("worldEnterButtonDisabledSprite_", worldEnterButtonDisabledSprite_);
        ImGuiHelper::OnDrawInputField("mapMarker_", mapMarker_);
        ImGuiHelper::OnDrawInputField("detailPreview_", detailPreview_);
        ImGuiHelper::OnDrawInputField("detailElement_", detailElement_);
        ImGuiHelper::OnDrawInputField("detailLabel_", detailLabel_);
        ImGuiHelper::OnDrawInputField("detailTitle_", detailTitle_);
        ImGuiHelper::OnDrawInputField("detailTag_", detailTag_);
        ImGuiHelper::OnDrawInputField("detailDifficulty_", detailDifficulty_);
        ImGuiHelper::OnDrawInputField("detailDescriptionLines_", detailDescriptionLines_, [this]
        {
            if (ImGui::Button("Add Description Line"))
            {
                detailDescriptionLines_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("noSelectionTitle_", noSelectionTitle_);
    }
}

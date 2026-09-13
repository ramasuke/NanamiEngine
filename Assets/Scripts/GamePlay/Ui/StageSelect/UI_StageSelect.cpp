#include "UI_StageSelect.h"

#include "../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../../../Engine/Core/Object/Field/CreateField.h"
#include "../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace GamePlay::Ui
{
    void StageSelectUi::OnAwake()
    {
        backGroundMask_            = GameObject::CatchChild<NanamiUi::BlendImageRenderer>(Entity(), backGroundMaskName_);
        stageSelectBackGroundMask_ = GameObject::CatchChild<NanamiUi::BlendImageRenderer>(Entity(), stageSelectBackGroundMaskName_);
        worldMovieRenderer_        = GameObject::CatchChild<NanamiUi::MovieRenderer>(Entity(), worldMovieRendererName_);
        worldEnterButton_          = GameObject::CatchChild<NanamiUi::Button>(Entity(), worldEnterButtonName_);
        worldEnterButtonGlow_      = GameObject::CatchChild<NanamiUi::ImageAnimationRenderer>(Entity(), worldEnterButtonName_);
        backGround_                = GameObject::CatchChild<NanamiUi::MovieRenderer>(Entity(), backGroundName_);
        mapMarker_                 = GameObject::CatchChild<StageMapMarker>(Entity(), mapMarkerName_);

        for (const auto& buttonName : stageSelectButtonNames_)
        {
            auto selectStageUi = GameObject::CatchChild<StageSelectStageUi>(Entity(), buttonName);
            stageSelectButtons_.push_back(CreateField<StageSelectStageUi>(selectStageUi));
        }
    }

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

    Coroutine::Task<void> StageSelectUi::StartStageSelectAsync()
    {
        co_await AppearBackGroundMaskAsync();
    }

    Coroutine::Task<void> StageSelectUi::AppearBackGroundMaskAsync()
    {
        Coroutine::StartCoroutine(FadeBlendRateAsync(backGround_.get(), 0, 255));
        co_await FadeBlendRateAsync(backGroundMask_.get(), 0, backGroundMaskBlendRate_);
    }

    Coroutine::Task<void> StageSelectUi::PlayEnterWorldTransitionAsync(const GameCore::Scene::Main::SceneType sceneType)
    {
        if (isEnteringWorld_)
            co_return;

        isEnteringWorld_ = true;
        Coroutine::StartCoroutine(FadeBlendRateAsync(stageSelectBackGroundMask_.get(), 0, stageSelectBackGroundMaskBlendRate_));
        co_await FadeBlendRateAsync(backGround_.get(), 255, 0);
        GameCore::Game::Instance().Scenes().RequestChangeScene(sceneType);
        isEnteringWorld_ = false;
    }

    Coroutine::Task<void> StageSelectUi::FadeBlendRateAsync(
        const std::shared_ptr<NanamiUi::BlendImageRenderer> renderer, const int from, const int to)
    {
        const int step = to > from ? 1 : -1;
        for (int rate = from; rate != to; rate += step)
        {
            renderer->SetBlendRate(rate);
            co_await Coroutine::WaitYield();
        }
        renderer->SetBlendRate(to);
    }

    Coroutine::Task<void> StageSelectUi::FadeBlendRateAsync(
        const std::shared_ptr<NanamiUi::MovieRenderer> renderer, const int from, const int to)
    {
        const int step = to > from ? 1 : -1;
        for (int rate = from; rate != to; rate += step)
        {
            renderer->SetBlendRate(rate);
            co_await Coroutine::WaitYield();
        }
        renderer->SetBlendRate(to);
    }

    void StageSelectUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("backGroundMaskName_", backGroundMaskName_);
        ImGuiHelper::OnDrawInputField("backGroundMaskBlendRate_", backGroundMaskBlendRate_);
        ImGuiHelper::OnDrawInputField("stageSelectButtonNames_", stageSelectButtonNames_, [this]
        {
            if (ImGui::Button("Add"))
            {
                stageSelectButtonNames_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("stageSelectBackGroundMaskName_", stageSelectBackGroundMaskName_);
        ImGuiHelper::OnDrawInputField("stageSelectBackGroundMask_", stageSelectBackGroundMask_);
        ImGuiHelper::OnDrawInputField("stageSelectBackGroundMaskBlendRate_", stageSelectBackGroundMaskBlendRate_);
        ImGuiHelper::OnDrawInputField("worldMovieRendererName_", worldMovieRendererName_);
        ImGuiHelper::OnDrawInputField("worldMovieRenderer_", worldMovieRenderer_);
        ImGuiHelper::OnDrawInputField("worldEnterButtonName_", worldEnterButtonName_);
        ImGuiHelper::OnDrawInputField("worldEnterButton_", worldEnterButton_);
        ImGuiHelper::OnDrawInputField("worldEnterButtonGlow_", worldEnterButtonGlow_);
        ImGuiHelper::OnDrawInputField("backGroundName_", backGroundName_);
        ImGuiHelper::OnDrawInputField("backGround_", backGround_);
        ImGuiHelper::OnDrawInputField("worldEnterButtonActiveSprite_", worldEnterButtonActiveSprite_);
        ImGuiHelper::OnDrawInputField("worldEnterButtonDisabledSprite_", worldEnterButtonDisabledSprite_);
        ImGuiHelper::OnDrawInputField("mapMarkerName_", mapMarkerName_);
        ImGuiHelper::OnDrawInputField("mapMarker_", mapMarker_);
    }
}

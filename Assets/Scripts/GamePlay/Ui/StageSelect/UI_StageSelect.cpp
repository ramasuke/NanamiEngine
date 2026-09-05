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
    }
    
    void StageSelectUi::OnStart()
    {
        for (const auto& buttonName : stageSelectButtonNames_)
        {
            auto selectStageUi = GameObject::CatchChild<StageSelectStageUi>(Entity(), buttonName);
            std::weak_ptr weakSelectStageUi = selectStageUi;
            selectStageUi->SubscribeOnClickSelectButton([this, weakSelectStageUi]
            {
                selectedSceneType_ = weakSelectStageUi.lock()->SceneType();
                hasSelectedSceneType_ = true;
            });
            stageSelectButtons_.push_back(CreateField<StageSelectStageUi>(selectStageUi));
        }

        worldEnterButton_->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            if (!hasSelectedSceneType_ || isEnteringWorld_)
                return;

            isEnteringWorld_ = true;
            Coroutine::StartCoroutine(EnterWorldAsync(selectedSceneType_));
        });
        
        Coroutine::StartCoroutine(StartStageSelectAsync());
    }

    void StageSelectUi::OnDestroy()
    {
        ComponentBase::OnDestroy();
    }

    Coroutine::Task<void> StageSelectUi::StartStageSelectAsync()
    {
        co_await AppearBackGroundMaskAsync();
    }

    Coroutine::Task<void> StageSelectUi::AppearBackGroundMaskAsync()
    {
        co_await FadeBlendRateAsync(backGroundMask_.get(), 0, backGroundMaskBlendRate_);
    }

    Coroutine::Task<void> StageSelectUi::EnterWorldAsync(const GameCore::Scene::Main::SceneType sceneType)
    {
        co_await FadeBlendRateAsync(stageSelectBackGroundMask_.get(), 0, stageSelectBackGroundMaskBlendRate_);
        GameCore::Game::Instance().Scenes().RequestChangeScene(sceneType);
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
        ImGui::Text("selectedSceneType_: %s", hasSelectedSceneType_
            ? GameCore::Scene::Main::ToString(selectedSceneType_).data()
            : "None");
    }
}

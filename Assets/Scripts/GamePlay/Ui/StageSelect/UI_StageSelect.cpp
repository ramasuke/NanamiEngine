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
        backGroundMask_ = GameObject::CatchChild<NanamiUi::BlendImageRenderer>(Entity(), backGroundMaskName_);
        for (const auto& buttonName : stageSelectButtonNames_)
        {
            auto selectStageUi = GameObject::CatchChild<StageSelectStageUi>(Entity(), buttonName);
            selectStageUi->SubscribeOnClickSelectButton([this, selectStageUi]
            {
                selectedSceneType_ = selectStageUi->SceneType();
                hasSelectedSceneType_ = true;
            });
            stageSelectButtons_.push_back(CreateField<StageSelectStageUi>(selectStageUi));
        }
        stageSelectBackGroundMask_ = GameObject::CatchChild<NanamiUi::BlendImageRenderer>(Entity(), stageSelectBackGroundMaskName_);
        worldMovieRenderer_        = GameObject::CatchChild<NanamiUi::MovieRenderer>(Entity(), worldMovieRendererName_);
        worldEnterButton_          = GameObject::CatchChild<NanamiUi::Button>(Entity(), worldEnterButtonName_);
        worldEnterButton_->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            if (!hasSelectedSceneType_)
                return;
            
            // GameCore::Game::Instance().Scenes().RequestChangeScene(selectedSceneType_);
        });
    }
    void StageSelectUi::OnStart() { Coroutine::StartCoroutine(StartStageSelectAsync()); }

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
        for (int i = 0; i < backGroundMaskBlendRate_; i++)
        {
            backGroundMask_->SetBlendRate(backGroundMaskBlendRate_);
            co_await Coroutine::WaitYield();
        }
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

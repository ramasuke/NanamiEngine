#include "UI_StageSelect.h"

#include "../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../../../Engine/Core/Object/Field/CreateField.h"
#include "../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"

namespace GamePlay::Ui
{
    void StageSelectUi::OnAwake()
    {
        backGroundMask_ = GameObject::CatchChild<NanamiUi::BlendImageRenderer>(Entity(), backGroundMaskName_);
        for (const auto& buttonName : stageSelectButtonNames_)
        {
            auto selectStageUi = GameObject::CatchChild<StageSelectStageUi>(Entity(), buttonName);
            stageSelectButtons_.push_back(CreateField<StageSelectStageUi>(selectStageUi));
        }
    }
    void StageSelectUi::OnStart() { Coroutine::StartCoroutine(StartStageSelectAsync()); }
    
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
        ImGuiHelper::OnDrawInputField("backGroundMaskName_", backGroundMaskName_);
        ImGuiHelper::OnDrawInputField("backGroundMaskBlendRate_", backGroundMaskBlendRate_);
        ImGuiHelper::OnDrawInputField("stageSelectButtonNames_", stageSelectButtonNames_, [this]
        {
            if (ImGui::Button("Add"))
            {
                stageSelectButtonNames_.emplace_back();
            }
        });
    }
}

#include "StageSelectPresenter.h"

#include "DxLib.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "../UI_StageSelect.h"

namespace GamePlay::Ui
{
    void StageSelectPresenter::OnStart()
    {
        view_  = RequireComponent<StageSelectUi>();
        model_ = std::make_unique<StageSelectModel>(view_->Stages());

        const auto& stages = model_->Stages();
        for (size_t i = 0; i < stages.size(); ++i)
        {
            if (const auto stage = stages[i].lock())
            {
                stage->SubscribeOnClickSelectButton([this, i]
                {
                    model_->SelectStage(i);
                });
            }
        }

        model_->OnSelectionChanged().Subscribe([this](const size_t index)
        {
            view_->HighlightSelectedStage(index);
            view_->SetWorldEnterButtonEnabled(true);

            if (const auto stage = model_->Stages()[index].lock())
            {
                view_->ShowMapMarker(stage->MapMarkerPosition(), stage->IsCleared());
                view_->ShowStageDetail(*stage->Data());
            }
        }).AddTo(this);

        view_->SetWorldEnterButtonEnabled(false);
        view_->ShowNoSelectionDetail();

        view_->OnWorldEnterButtonClicked().Subscribe([this](NanamiUi::MouseState)
        {
            TryEnterWorld();
        }).AddTo(this);
    }

    void StageSelectPresenter::OnUpdate()
    {
        XINPUT_STATE xInput{};
        GetJoypadXInputState(DX_INPUT_PAD1, &xInput);
        const bool isConfirmPressed = CheckHitKey(KEY_INPUT_RETURN) || CheckHitKey(KEY_INPUT_SPACE) || xInput.Buttons[XINPUT_BUTTON_A];

        if (isConfirmPressed && !wasConfirmPressed_)
            TryEnterWorld();

        wasConfirmPressed_ = isConfirmPressed;
    }

    void StageSelectPresenter::TryEnterWorld()
    {
        if (!model_ || !model_->HasSelection())
            return;

        view_->EnterWorld(model_->SelectedSceneType());
    }
}

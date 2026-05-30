#include "Ui_StageSelect_StageUI.h"

#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Sound/SoundPlayer.h"

namespace GamePlay::Ui
{
    void StageSelectStageUi::SubscribeOnClickSelectButton(std::function<void()> onClick)
    {
        selectButton_->OnClick().subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        });
    }

    void StageSelectStageUi::OnAwake()
    {
        selectButton_ = RequireComponent<NanamiUi::Button>();
        selectButton_->OnHover().subscribe([this](auto)
        {
            Sound::SoundPlayer::PlaySe(*selectButtonHoverSound_.get(), Sound::SoundPlayer::Position());
        });
        selectButton_->OnClick().subscribe([this](NanamiUi::MouseState)
        {
            Sound::SoundPlayer::PlaySe(*selectButtonClickSound_.get(), Sound::SoundPlayer::Position());
        });
    }

    void StageSelectStageUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("selectButtonHoverSound_", selectButtonHoverSound_);
        ImGuiHelper::OnDrawInputField("selectButtonClickSound_", selectButtonClickSound_);
        ImGuiHelper::OnDrawEnumField("sceneType_", stageSceneType_, GameCore::Scene::Main::SCENE_TYPES, GameCore::Scene::Main::ToString);
    }
}

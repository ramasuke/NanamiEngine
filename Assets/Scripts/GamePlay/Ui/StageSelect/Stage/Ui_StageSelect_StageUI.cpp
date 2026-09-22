#include "Ui_StageSelect_StageUI.h"

#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Sound/SoundPlayer.h"

namespace GamePlay::Ui
{
    void StageSelectStageUi::SubscribeOnClickSelectButton(std::function<void()> onClick)
    {
        selectButton_->OnClick().Subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        }).AddTo(this);
    }

    void StageSelectStageUi::SetHighlighted(const bool isHighlighted)
    {
        isHighlighted_ = isHighlighted;
        RefreshAppearance();
    }

    void StageSelectStageUi::RefreshAppearance()
    {
        // 選択中の行は枠グローが出るので、枠入りのホバー用スプライトは重ねない
        const bool showHoverSprite = isHovering_ && !isHighlighted_;
        imageRenderer_->SetSprite(showHoverSprite ? selectedRowSprite_.get() : unselectedRowSprite_.get());
        glowAnimation_->SetEnable(isHighlighted_);
    }

    void StageSelectStageUi::OnAwake()
    {
        selectButton_   = RequireComponent<NanamiUi::Button>();
        imageRenderer_  = RequireComponent<Component::ImageRenderer>();
        glowAnimation_  = RequireComponent<NanamiUi::ImageAnimationRenderer>();
        nameText_->SetText(stageData_->DisplayName());
        elementIcon_->SetSprite(stageData_->ElementSprite());
        difficultyPips_->SetDifficulty(stageData_->Difficulty());
        SetHighlighted(false);

        selectButton_->OnHover().Subscribe([this](auto)
        {
            Sound::SoundPlayer::PlaySe(*selectButtonHoverSound_.get(), Sound::SoundPlayer::Position());
            isHovering_ = true;
            RefreshAppearance();
        }).AddTo(this);
        selectButton_->OnHoverExit().Subscribe([this](auto)
        {
            isHovering_ = false;
            RefreshAppearance();
        }).AddTo(this);
        selectButton_->OnClick().Subscribe([this](NanamiUi::MouseState)
        {
            Sound::SoundPlayer::PlaySe(*selectButtonClickSound_.get(), Sound::SoundPlayer::Position());
        }).AddTo(this);
    }

    void StageSelectStageUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("selectButtonHoverSound_", selectButtonHoverSound_);
        ImGuiHelper::OnDrawInputField("selectButtonClickSound_", selectButtonClickSound_);
        ImGuiHelper::OnDrawInputField("stageData_", stageData_);
        ImGuiHelper::OnDrawInputField("selectedRowSprite_", selectedRowSprite_);
        ImGuiHelper::OnDrawInputField("unselectedRowSprite_", unselectedRowSprite_);
        ImGuiHelper::OnDrawInputField("elementIcon_", elementIcon_);
        ImGuiHelper::OnDrawInputField("difficultyPips_", difficultyPips_);
    }
}

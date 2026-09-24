#include "Ui_StageSelect_StageUI.h"

#include "../../../../Core/Game/Game.h"
#include "../../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Sound/UiSoundBank.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

    void StageSelectStageUi::SetLocked(const bool isLocked)
    {
        isLocked_ = isLocked;
        nameText_->SetText(isLocked ? "？？？" : stageData_->DisplayName());
        elementIcon_->SetSprite(isLocked ? lockedElementSprite_.get() : stageData_->ElementSprite());
        if (const auto entity = difficultyPips_->Entity().lock())
        {
            entity->SetEnable(!isLocked);
        }
        // NOTE: OnAwake より先に呼ばれたときは、見た目は OnAwake で合わせる
        if (imageRenderer_)
            RefreshAppearance();
    }

    void StageSelectStageUi::RefreshAppearance()
    {
        // 選択中の行は枠グローが出るので、枠入りのホバー用スプライトは重ねない
        const bool showHoverSprite = isHovering_ && !isHighlighted_;
        const auto sprite = isLocked_
            ? lockedRowSprite_.get()
            : (showHoverSprite ? selectedRowSprite_.get() : unselectedRowSprite_.get());
        imageRenderer_->SetSprite(sprite);
        glowAnimation_->SetEnable(isHighlighted_);
    }

    void StageSelectStageUi::OnAwake()
    {
        selectButton_   = RequireComponent<NanamiUi::Button>();
        imageRenderer_  = RequireComponent<Component::ImageRenderer>();
        glowAnimation_  = RequireComponent<NanamiUi::ImageAnimationRenderer>();
        difficultyPips_->SetDifficulty(stageData_->Difficulty());
        SetLocked(isLocked_);
        SetHighlighted(false);

        selectButton_->OnHover().Subscribe([this](auto)
        {
            Sound::UiSoundBank::Play(uiSounds_, selectButtonHoverSound_, Sound::UiSe::Cursor);
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
            Sound::UiSoundBank::Play(uiSounds_, selectButtonClickSound_, Sound::UiSe::Confirm);
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
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGuiHelper::OnDrawInputField("lockedRowSprite_", lockedRowSprite_);
        ImGuiHelper::OnDrawInputField("lockedElementSprite_", lockedElementSprite_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageSelectStageUi);
#pragma endregion

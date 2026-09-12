#include "Ui_StageSelect_StageUI.h"

#include "../../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
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

    void StageSelectStageUi::SetHighlighted(const bool isHighlighted)
    {
        imageRenderer_->SetSprite(isHighlighted ? selectedRowSprite_.get() : unselectedRowSprite_.get());
        glowAnimation_->SetEnable(isHighlighted);
    }

    void StageSelectStageUi::OnAwake()
    {
        selectButton_   = RequireComponent<NanamiUi::Button>();
        imageRenderer_  = RequireComponent<Component::ImageRenderer>();
        glowAnimation_  = RequireComponent<NanamiUi::ImageAnimationRenderer>();
        nameText_       = GameObject::CatchChild<NanamiUi::TextRenderer>(Entity(), nameTextChildName_);
        nameText_->SetText(stageData_->DisplayName());
        SetHighlighted(false);

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
        ImGuiHelper::OnDrawInputField("nameTextChildName_", nameTextChildName_);
        ImGuiHelper::OnDrawInputField("selectButtonHoverSound_", selectButtonHoverSound_);
        ImGuiHelper::OnDrawInputField("selectButtonClickSound_", selectButtonClickSound_);
        ImGuiHelper::OnDrawInputField("stageData_", stageData_);
        ImGuiHelper::OnDrawInputField("selectedRowSprite_", selectedRowSprite_);
        ImGuiHelper::OnDrawInputField("unselectedRowSprite_", unselectedRowSprite_);
    }
}

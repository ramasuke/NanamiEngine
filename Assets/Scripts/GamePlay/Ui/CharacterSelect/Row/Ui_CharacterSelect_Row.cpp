#include "Ui_CharacterSelect_Row.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Sound/SoundPlayer.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void CharacterSelectRow::EnsureComponents()
    {
        if (selectButton_ && billRenderer_)
            return;

        selectButton_ = RequireComponent<NanamiUi::Button>();
        billRenderer_ = RequireComponent<Component::ImageRenderer>();
        baseScale_    = Transform().GetLocalScale();
    }

    void CharacterSelectRow::OnAwake()
    {
        EnsureComponents();

        selectButton_->OnHover().Subscribe([this](auto)
        {
            if (const auto sound = hoverSound_.get())
                Sound::SoundPlayer::PlaySe(*sound, Sound::SoundPlayer::Position());
            isHovering_ = true;
            RefreshAppearance();
        }).AddTo(this);
        selectButton_->OnHoverExit().Subscribe([this](auto)
        {
            isHovering_ = false;
            RefreshAppearance();
        }).AddTo(this);
    }

    void CharacterSelectRow::Initialize(const std::shared_ptr<Asset::CharacterData>& character)
    {
        EnsureComponents();
        character_ = character;

        nameText_   ->SetText(character->DisplayName());
        readingText_->SetText(character->Reading());
        taglineText_->SetText(character->Tagline());
        lockedStamp_->SetEnable(!character->IsUnlocked());

        SetHighlighted(false);
    }

    void CharacterSelectRow::SubscribeOnClickSelectButton(std::function<void()> onClick)
    {
        EnsureComponents();
        selectButton_->OnClick().Subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        }).AddTo(this);
    }

    void CharacterSelectRow::SetHighlighted(const bool isHighlighted)
    {
        isHighlighted_ = isHighlighted;
        RefreshAppearance();
    }

    void CharacterSelectRow::RefreshAppearance() const
    {
        const bool isLocked = character_ && !character_->IsUnlocked();
        const auto sprite = isLocked
            ? lockedBillSprite_.get()
            : (isHighlighted_ || isHovering_ ? selectedBillSprite_.get() : unselectedBillSprite_.get());
        billRenderer_->SetSprite(sprite);

        // 蝋の封は「いま選ばれている一枚」の印なので、ロック中には出さない
        waxSeal_->SetEnable(isHighlighted_ && !isLocked);
        Transform().SetLocalScale(isHighlighted_ ? baseScale_ * selectedScale_ : baseScale_);
    }

    void CharacterSelectRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("readingText_", readingText_);
        ImGuiHelper::OnDrawInputField("taglineText_", taglineText_);
        ImGuiHelper::OnDrawInputField("waxSeal_", waxSeal_);
        ImGuiHelper::OnDrawInputField("lockedStamp_", lockedStamp_);
        ImGuiHelper::OnDrawInputField("selectedBillSprite_", selectedBillSprite_);
        ImGuiHelper::OnDrawInputField("unselectedBillSprite_", unselectedBillSprite_);
        ImGuiHelper::OnDrawInputField("lockedBillSprite_", lockedBillSprite_);
        ImGuiHelper::OnDrawInputField("hoverSound_", hoverSound_);
        ImGuiHelper::OnDrawInputField("selectedScale_", selectedScale_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::CharacterSelectRow);
#pragma endregion

#include "Ui_EventBoard_Tab.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void EventBoardTab::EnsureComponents()
    {
        if (selectButton_ && boardRenderer_)
            return;

        selectButton_  = RequireComponent<NanamiUi::Button>();
        boardRenderer_ = RequireComponent<Component::ImageRenderer>();
        basePos_       = Transform().GetLocalPos();
        baseScale_     = Transform().GetLocalScale();
    }

    void EventBoardTab::OnAwake()
    {
        EnsureComponents();
    }

    void EventBoardTab::Place(const glm::vec3& localPos)
    {
        EnsureComponents();
        basePos_ = localPos;
        Transform().SetLocalPos(basePos_);
    }

    void EventBoardTab::SetLabel(const std::string& label)
    {
        labelText_->SetText(label);
    }

    void EventBoardTab::SetSelected(const bool isSelected)
    {
        EnsureComponents();

        boardRenderer_->SetSprite(isSelected ? selectedSprite_.get() : unselectedSprite_.get());
        labelText_->SetTextColor(isSelected ? selectedLabelColor_ : unselectedLabelColor_);
        Transform().SetLocalScale(isSelected ? baseScale_ * selectedScale_ : baseScale_);
        Transform().SetLocalPos(isSelected ? basePos_ + glm::vec3(0.0f, selectedDrop_px_, 0.0f) : basePos_);
    }

    void EventBoardTab::SetBadgeCount(const size_t count)
    {
        if (const auto badge = badgeRoot_.get())
            badge->SetEnable(count > 0);
        if (count > 0)
            badgeCountText_->SetText(std::to_string(count));
    }

    void EventBoardTab::SubscribeOnClick(std::function<void()> onClick)
    {
        EnsureComponents();
        selectButton_->OnClick().Subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        }).AddTo(this);
    }

    void EventBoardTab::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("labelText_", labelText_);
        ImGuiHelper::OnDrawInputField("badgeRoot_", badgeRoot_);
        ImGuiHelper::OnDrawInputField("badgeCountText_", badgeCountText_);
        ImGuiHelper::OnDrawInputField("selectedSprite_", selectedSprite_);
        ImGuiHelper::OnDrawInputField("unselectedSprite_", unselectedSprite_);
        ImGuiHelper::OnDrawInputField("selectedLabelColor_", selectedLabelColor_);
        ImGuiHelper::OnDrawInputField("unselectedLabelColor_", unselectedLabelColor_);
        ImGuiHelper::OnDrawInputField("selectedScale_", selectedScale_);
        ImGuiHelper::OnDrawInputField("selectedDrop_px_", selectedDrop_px_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardTab);
#pragma endregion

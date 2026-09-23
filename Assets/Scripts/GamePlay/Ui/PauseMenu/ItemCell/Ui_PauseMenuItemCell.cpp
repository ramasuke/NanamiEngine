#include "Ui_PauseMenuItemCell.h"

#include <string>
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void PauseMenuItemCell::SetContent(const std::weak_ptr<Asset::SpriteFile>& icon, const int count) const
    {
        if (icon_)      icon_     ->SetSprite(icon);
        if (countText_) countText_->SetText("×" + std::to_string(count));
    }

    void PauseMenuItemCell::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("icon_", icon_);
        ImGuiHelper::OnDrawInputField("countText_", countText_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::PauseMenuItemCell);
#pragma endregion

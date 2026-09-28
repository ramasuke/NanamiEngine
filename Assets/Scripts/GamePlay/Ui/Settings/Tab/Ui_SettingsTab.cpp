#include "Ui_SettingsTab.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void SettingsTabUi::Show(const std::string& name)
    {
        if (const auto text = nameText_.get())
            text->SetText(name);
    }

    void SettingsTabUi::SetSelected(const bool isSelected)
    {
        if (const auto text = nameText_.get())
            text->SetTextColor(isSelected ? selectedColor_ : unselectedColor_);
        if (const auto plate = plate_.get())
            plate->SetEnable(!isSelected);
        if (const auto plate = selectedPlate_.get())
            plate->SetEnable(isSelected);
    }

    void SettingsTabUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("plate_", plate_);
        ImGuiHelper::OnDrawInputField("selectedPlate_", selectedPlate_);
        ImGuiHelper::OnDrawInputField("selectedColor_", selectedColor_);
        ImGuiHelper::OnDrawInputField("unselectedColor_", unselectedColor_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SettingsTabUi);
#pragma endregion

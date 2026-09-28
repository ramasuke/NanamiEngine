#include "Ui_SettingsRow.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void SettingsRowUi::Show(const std::string& label, const std::string& value)
    {
        if (const auto text = labelText_.get())
            text->SetText(label);
        if (const auto text = valueText_.get())
            text->SetText(value);
    }

    void SettingsRowUi::SetSelected(const bool isSelected)
    {
        const Color32 color = isSelected ? selectedColor_ : unselectedColor_;
        if (const auto text = labelText_.get())
            text->SetTextColor(color);
        if (const auto text = valueText_.get())
            text->SetTextColor(color);
        if (const auto band = band_.get())
            band->SetEnable(isSelected);
        if (const auto arrow = leftArrow_.get())
            arrow->SetEnable(isSelected);
        if (const auto arrow = rightArrow_.get())
            arrow->SetEnable(isSelected);
    }

    void SettingsRowUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("labelText_", labelText_);
        ImGuiHelper::OnDrawInputField("valueText_", valueText_);
        ImGuiHelper::OnDrawInputField("band_", band_);
        ImGuiHelper::OnDrawInputField("leftArrow_", leftArrow_);
        ImGuiHelper::OnDrawInputField("rightArrow_", rightArrow_);
        ImGuiHelper::OnDrawInputField("selectedColor_", selectedColor_);
        ImGuiHelper::OnDrawInputField("unselectedColor_", unselectedColor_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SettingsRowUi);
#pragma endregion

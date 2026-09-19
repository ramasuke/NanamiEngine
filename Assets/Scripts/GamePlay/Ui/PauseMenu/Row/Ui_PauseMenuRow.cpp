#include "Ui_PauseMenuRow.h"

namespace GamePlay::Ui
{
    void PauseMenuRow::SetContent(const std::string& name, const std::string& description, const int number) const
    {
        if (nameText_)        nameText_       ->SetText(name);
        if (descriptionText_) descriptionText_->SetText(description);
        if (numberText_)      numberText_     ->SetText(std::to_string(number));
    }

    void PauseMenuRow::SetHighlighted(const bool isHighlighted) const
    {
        if (marker_)    marker_   ->SetEnable(isHighlighted);
        if (underline_) underline_->SetEnable(isHighlighted);
    }

    void PauseMenuRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("descriptionText_", descriptionText_);
        ImGuiHelper::OnDrawInputField("numberText_", numberText_);
        ImGuiHelper::OnDrawInputField("marker_", marker_);
        ImGuiHelper::OnDrawInputField("underline_", underline_);
    }
}

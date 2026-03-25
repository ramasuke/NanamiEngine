#include "Ui_SwordMan_ActionInstructTutorial.h"

namespace GamePlay::Ui
{
    void SwordManActionInstructTutorial::Hide()
    {
        Entity().lock()->SetEnable(false);
    }

    void SwordManActionInstructTutorial::OnDisplayText(const std::string& text) const
    {
        Entity().lock()->SetEnable(true);
        
        textBox_->SetText(text);
    }

    void SwordManActionInstructTutorial::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("textBox_", textBox_);
    }
}

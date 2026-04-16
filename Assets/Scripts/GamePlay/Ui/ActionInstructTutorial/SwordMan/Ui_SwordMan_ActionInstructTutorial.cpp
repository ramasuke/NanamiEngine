#include "Ui_SwordMan_ActionInstructTutorial.h"

#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Ui
{
    void SwordManActionInstructTutorial::Hide()
    {
        Entity().lock()->SetEnable(false);
    }

    void SwordManActionInstructTutorial::OnDisplayText(const std::string& text)
    {
        Entity().lock()->SetEnable(true);

        const auto textBox = Transform().CatchChild(textBoxName_);
        textBox_ = textBox->Components().Catch<NanamiUi::TextRenderer>();
        textBox_->SetText(text);
    }

    void SwordManActionInstructTutorial::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("textBox_", textBox_);
        ImGuiHelper::OnDrawInputField("textBoxName_", textBoxName_);
        ImGuiHelper::OnDrawInputField("attackButtonTutorialText_", attackButtonTutorialText_);
        ImGuiHelper::OnDrawInputField("runButtonTutorialText_", runButtonTutorialText_);
        ImGuiHelper::OnDrawInputField("dashAttackButtonTutorialText_", dashAttackButtonTutorialText_);
    }
}

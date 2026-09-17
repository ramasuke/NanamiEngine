#include "StageDifficultyPips.h"

#include "../../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Ui
{
    void StageDifficultyPips::SetDifficulty(const int difficulty)
    {
        const auto pips = Transform().GetChildren();
        for (size_t i = 0; i < pips.size(); ++i)
        {
            if (const auto renderer = pips[i]->Components().Catch<Component::ImageRenderer>().lock())
            {
                renderer->SetSprite(static_cast<int>(i) < difficulty ? filledSprite_.get() : emptySprite_.get());
            }
        }
    }

    void StageDifficultyPips::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("filledSprite_", filledSprite_);
        ImGuiHelper::OnDrawInputField("emptySprite_", emptySprite_);
    }
}

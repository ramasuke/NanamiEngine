#include "VerticalLayoutGroup.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../GameObject/Interface/IGameObject.h"

namespace NanamiEngine::Module::NanamiUi
{
    void VerticalLayoutGroup::OnLateUpdate()
    {
        if (!IsEnable())
            return;

        const auto children = Transform().GetChildren();
        const std::size_t count = children.size();
        if (count == 0)
            return;

        const float offsetX = ToCrossAlignFactor(childAlignment_) * cellSize_.x;

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t slot = reverseArrangement_ ? (count - 1 - i) : i;
            auto& childTransform = children[i]->Transform();
            const glm::vec3& current = childTransform.GetLocalPos();
            childTransform.SetLocalPos(glm::vec3(
                offsetX,
                static_cast<float>(slot) * (cellSize_.y + spacing_),
                current.z
            ));
        }
    }

    void VerticalLayoutGroup::OnDrawGui()
    {
        float cellSize[2] = { cellSize_.x, cellSize_.y };
        if (ImGui::InputFloat2("cellSize_", cellSize))
        {
            cellSize_.x = cellSize[0];
            cellSize_.y = cellSize[1];
        }
        ImGuiHelper::OnDrawInputField("spacing_", spacing_);
        ImGuiHelper::OnDrawEnumField("childAlignment_", childAlignment_, LAYOUT_CROSS_ALIGNS, ToString);
        ImGuiHelper::OnDrawInputField("reverseArrangement_", reverseArrangement_);
    }
}

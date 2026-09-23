#include "GridLayoutGroup.h"
#include <algorithm>
#include "../../GameObject/Transform/Transform.h"
#include "../../GameObject/Interface/IGameObject.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::NanamiUi
{
    void GridLayoutGroup::OnLateUpdate()
    {
        if (!IsEnable())
            return;

        const auto children = Transform().GetChildren();
        const std::size_t count = children.size();
        if (count == 0)
            return;

        const int fixedCount = std::max(1, constraintCount_);

        for (std::size_t i = 0; i < count; ++i)
        {
            int row, col;
            if (constraint_ == GridConstraint::FixedRowCount)
            {
                row = static_cast<int>(i) % fixedCount;
                col = static_cast<int>(i) / fixedCount;
            }
            else
            {
                col = static_cast<int>(i) % fixedCount;
                row = static_cast<int>(i) / fixedCount;
            }

            float x = static_cast<float>(col) * (cellSize_.x + spacing_.x);
            float y = static_cast<float>(row) * (cellSize_.y + spacing_.y);

            if (startCorner_ == GridStartCorner::UpperRight || startCorner_ == GridStartCorner::LowerRight)
                x = -x;
            if (startCorner_ == GridStartCorner::LowerLeft || startCorner_ == GridStartCorner::LowerRight)
                y = -y;

            auto& childTransform = children[i]->Transform();
            const glm::vec3& current = childTransform.GetLocalPos();
            childTransform.SetLocalPos(glm::vec3(x, y, current.z));
        }
    }

    void GridLayoutGroup::OnDrawGui()
    {
        float cellSize[2] = { cellSize_.x, cellSize_.y };
        if (ImGui::InputFloat2("cellSize_", cellSize))
        {
            cellSize_.x = cellSize[0];
            cellSize_.y = cellSize[1];
        }
        float spacing[2] = { spacing_.x, spacing_.y };
        if (ImGui::InputFloat2("spacing_", spacing))
        {
            spacing_.x = spacing[0];
            spacing_.y = spacing[1];
        }
        ImGuiHelper::OnDrawEnumField("constraint_", constraint_, GRID_CONSTRAINTS, ToString);
        ImGuiHelper::OnDrawInputField("constraintCount_", constraintCount_);
        ImGuiHelper::OnDrawEnumField("startCorner_", startCorner_, GRID_START_CORNERS, ToString);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::GridLayoutGroup);
#pragma endregion

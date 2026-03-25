#include "AnimationNodePathAdditionConditionGroup.h"
#include "../AdditionCondition/AnimationNodePathAdditionCondition.h"

#include "../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

void NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionConditionGroup::OnDrawGui()
{
    for (auto& condition : conditions_)
    {
        if (condition)
            condition->OnDrawGui();
    }

    if (ImGui::Button("Add Condition"))
    {
        ImGui::OpenPopup("AddConditionPopup");
    }

    if (ImGui::BeginPopup("AddConditionPopup"))
    {
        static int selectedType = 0;
        const char* types[] = { "Bool", "Int", "Float" };

        ImGui::Text("Select Condition Type:");
        ImGui::Combo("Type", &selectedType, types, IM_ARRAYSIZE(types));

        if (ImGui::Button("Confirm"))
        {
            switch (selectedType)
            {
            case 0:
                conditions_.push_back(std::make_shared<AnimationNodePathAdditionCondition<bool>>());
                break;
            case 1:
                conditions_.push_back(std::make_shared<AnimationNodePathAdditionCondition<int>>());
                break;
            case 2:
                conditions_.push_back(std::make_shared<AnimationNodePathAdditionCondition<float>>());
                break;
            default:
                break;
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionConditionGroup::Check(
    BlackBoard::ParameterGroup& additionParameterGroup) const
{
    for (const auto& condition : conditions_)
    {
        if (!condition || !condition->Check(additionParameterGroup))
            return false;
    }
    return true;
}

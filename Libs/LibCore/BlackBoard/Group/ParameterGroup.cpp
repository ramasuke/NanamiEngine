#include "ParameterGroup.h"

#include "../AnimationParameter.h"
#include "../../ImGui/Helper/ImGuiHelper.h"

void NanamiEngine::Module::BlackBoard::ParameterGroup::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("parameters", conditionParameters_, [this]
    {
        if (ImGui::Button("Add"))
        {
            ImGui::OpenPopup("AddParameterPopup");
        }

        if (ImGui::BeginPopup("AddParameterPopup"))
        {
            static int selectedType = 0;
            const char* types[] = { "Bool", "Int", "Float" };

            ImGui::Text("Select Parameter Type:");
            ImGui::Combo("Type", &selectedType, types, IM_ARRAYSIZE(types));

            if (ImGui::Button("Confirm"))
            {
                switch (selectedType)
                {
                case 0:
                    conditionParameters_.push_back(std::make_shared<AnimationTree::AnimationParameter<bool>>());
                    break;
                case 1:
                    conditionParameters_.push_back(std::make_shared<AnimationTree::AnimationParameter<int>>());
                    break;
                case 2:
                    conditionParameters_.push_back(std::make_shared<AnimationTree::AnimationParameter<float>>());
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
    });
}
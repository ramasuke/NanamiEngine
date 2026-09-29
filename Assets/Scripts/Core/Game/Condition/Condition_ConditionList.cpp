#include "Condition_ConditionList.h"

#include <algorithm>

#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Condition
{
    std::shared_ptr<ICondition> ConditionList::DrawCreateCombo(const char* label)
    {
        std::shared_ptr<ICondition> created;
        if (!ImGui::BeginCombo(label, "Add..."))
            return created;

        for (const auto& [name, create] : ConditionFactory::Instance().CreatableConditions())
        {
            if (ImGui::Selectable(name.c_str()))
                created = create();
        }
        ImGui::EndCombo();
        return created;
    }

    bool ConditionList::DrawConditionNode(const std::shared_ptr<ICondition>& condition)
    {
        const bool isOpen = ImGui::TreeNodeEx("##condition", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap,
                                              "%s", condition->Describe().c_str());
        ImGui::SameLine();
        const bool isRemoved = ImGui::SmallButton("Remove");
        if (isOpen)
        {
            condition->OnDrawGui();
            ImGui::TreePop();
        }
        return isRemoved;
    }

    bool ConditionList::AreAllSatisfied(const Conditions& conditions, const ConditionContext& context)
    {
        return std::ranges::all_of(conditions, [&context](const auto& condition)
        {
            return !condition || condition->IsSatisfied(context);
        });
    }

    void ConditionList::DrawListGui(const std::string& label, Conditions& conditions)
    {
        ImGui::PushID(label.c_str());
        ImGui::TextUnformatted(label.c_str());
        ImGui::Indent();

        for (size_t i = 0; i < conditions.size();)
        {
            ImGui::PushID(static_cast<int>(i));
            const bool isRemoved = !conditions[i] || DrawConditionNode(conditions[i]);
            ImGui::PopID();

            if (isRemoved)
                conditions.erase(conditions.begin() + static_cast<std::ptrdiff_t>(i));
            else
                ++i;
        }

        if (const auto created = DrawCreateCombo("##add"))
            conditions.push_back(created);

        ImGui::Unindent();
        ImGui::PopID();
    }

    void ConditionList::DrawSingleGui(const std::string& label, std::shared_ptr<ICondition>& condition)
    {
        ImGui::PushID(label.c_str());
        ImGui::TextUnformatted(label.c_str());
        ImGui::Indent();

        if (condition)
        {
            if (DrawConditionNode(condition))
                condition.reset();
        }
        else if (const auto created = DrawCreateCombo("##set"))
        {
            condition = created;
        }

        ImGui::Unindent();
        ImGui::PopID();
    }
}

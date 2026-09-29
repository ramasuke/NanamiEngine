#include "Data_NavigationGuide.h"
#include "../../Scripts/Core/Game/Condition/Condition_ConditionList.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    namespace
    {
        /** @brief 上下の入れ替えと削除のボタン。@return 消すなら true */
        template<typename T>
        bool DrawReorderButtons(std::vector<T>& items, const std::size_t index)
        {
            if (index > 0 && ImGui::SmallButton("Up"))
                std::swap(items[index], items[index - 1]);
            ImGui::SameLine();
            if (index + 1 < items.size() && ImGui::SmallButton("Down"))
                std::swap(items[index], items[index + 1]);
            ImGui::SameLine();
            return ImGui::SmallButton("Remove");
        }
    }

    void NavigationTargetOption::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("targetId_", targetId_);
        LibCore::ImGuiHelper::OnDrawInputField("title_", title_);
        LibCore::ImGuiHelper::OnDrawInputField("label_", label_);
    }

    bool NavigationStep::IsCurrent(const GameCore::Condition::ConditionContext& context) const
    {
        if (!GameCore::Condition::ConditionList::AreAllSatisfied(activeConditions_, context))
            return false;

        // NOTE: 空の doneConditions_ は「終わらない」。AreAllSatisfied は空で true を返すので分けて見る
        return doneConditions_.empty() || !GameCore::Condition::ConditionList::AreAllSatisfied(doneConditions_, context);
    }

    void NavigationStep::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("id_", id_);
        LibCore::ImGuiHelper::OnDrawInputField("title_", title_);
        GameCore::Condition::ConditionList::DrawListGui("activeConditions_", activeConditions_);
        GameCore::Condition::ConditionList::DrawListGui("doneConditions_", doneConditions_);

        if (!ImGui::TreeNode("targets_"))
            return;

        for (std::size_t i = 0; i < targets_.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            targets_[i].OnDrawGui();
            const bool remove = DrawReorderButtons(targets_, i);
            ImGui::Separator();
            ImGui::PopID();
            if (remove)
            {
                targets_.erase(targets_.begin() + static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
        if (ImGui::Button("Add Target"))
            targets_.emplace_back();
        ImGui::TreePop();
    }

    NavigationGuide::NavigationGuide(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    const NavigationStep* NavigationGuide::FindCurrent(const GameCore::Condition::ConditionContext& context) const
    {
        for (const auto& step : steps_)
        {
            if (step.IsCurrent(context))
                return &step;
        }
        return nullptr;
    }

    void NavigationGuide::OnDrawGui()
    {
        for (std::size_t i = 0; i < steps_.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            const bool open   = ImGui::TreeNode("step", "%zu: %s", i, steps_[i].id_.c_str());
            const bool remove = DrawReorderButtons(steps_, i);
            if (open)
            {
                steps_[i].OnDrawGui();
                ImGui::TreePop();
            }
            ImGui::PopID();
            if (remove)
            {
                steps_.erase(steps_.begin() + static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
        if (ImGui::Button("Add Step"))
            steps_.emplace_back();
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(NavigationGuide, NAVIGATION_GUIDE_EXTENSION_LABEL, "Navigation")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::NavigationGuide, NanamiEngine::Module::ScriptableObject);
#pragma endregion

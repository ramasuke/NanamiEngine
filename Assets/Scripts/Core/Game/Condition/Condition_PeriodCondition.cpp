#include "Condition_PeriodCondition.h"

#include "Condition_BoardTime.h"
#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool PeriodCondition::IsSatisfied(const ConditionContext& context) const
    {
        if (!context.now)
            return false;

        if (!startAt_.empty())
        {
            const auto start = ParseBoardTime(startAt_);
            if (!start || *context.now < *start)
                return false;
        }
        if (!endAt_.empty())
        {
            const auto end = ParseBoardTime(endAt_);
            if (!end || *end <= *context.now)
                return false;
        }
        return true;
    }

    std::string PeriodCondition::Describe() const
    {
        return "Period " + startAt_ + " ~ " + endAt_;
    }

    void PeriodCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("startAt_", startAt_);
        DrawBoardTimeWarning(startAt_);
        LibCore::ImGuiHelper::OnDrawInputField("endAt_", endAt_);
        DrawBoardTimeWarning(endAt_);

        const auto start = ParseBoardTime(startAt_);
        const auto end   = ParseBoardTime(endAt_);
        if (start && end && *end <= *start)
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "終了が開始より前です");
    }

    REGISTER_CONDITION(PeriodCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::PeriodCondition, GameCore::Condition::ICondition);

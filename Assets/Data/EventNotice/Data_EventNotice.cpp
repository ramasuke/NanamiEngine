#include "Data_EventNotice.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    EventNotice::EventNotice(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    std::optional<std::chrono::sys_seconds> EventNotice::StartTime() const
    {
        return ParseBoardTime(startAt_);
    }

    std::optional<std::chrono::sys_seconds> EventNotice::EndTime() const
    {
        return ParseBoardTime(endAt_);
    }

    bool EventNotice::IsOngoing(const std::chrono::sys_seconds now) const
    {
        const auto start = StartTime();
        const auto end   = EndTime();
        return start && end && *start <= now && now < *end;
    }

    bool EventNotice::IsUnlocked(const GameCore::Condition::ConditionContext& context) const
    {
        return GameCore::Condition::ConditionList::AreAllSatisfied(unlockConditions_, context);
    }

    void EventNotice::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("title_", title_);
        LibCore::ImGuiHelper::OnDrawInputField("tagText_", tagText_);
        LibCore::ImGuiHelper::OnDrawInputField("startAt_", startAt_);
        DrawBoardTimeWarning(startAt_);
        LibCore::ImGuiHelper::OnDrawInputField("endAt_", endAt_);
        DrawBoardTimeWarning(endAt_);

        const auto start = StartTime();
        const auto end   = EndTime();
        if (start && end && *end <= *start)
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "終了が開始より前です");

        LibCore::ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                descriptionLines_.emplace_back();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("bannerSprite_", bannerSprite_);
        GameCore::Condition::ConditionList::DrawListGui("unlockConditions_", unlockConditions_);
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(EventNotice, EVENT_NOTICE_EXTENSION_LABEL, "EventBoard")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::EventNotice, NanamiEngine::Module::ScriptableObject);
#pragma endregion

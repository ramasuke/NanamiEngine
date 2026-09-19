#include "Data_EventNotice.h"

#include <cstdio>

namespace NanamiEngine::Module::Asset
{
    std::optional<std::chrono::sys_seconds> ParseBoardTime(const std::string& text)
    {
        int year = 0, month = 0, day = 0, hour = 0, minute = 0;
        if (sscanf_s(text.c_str(), "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) != 5)
            return std::nullopt;

        const std::chrono::year_month_day date{
            std::chrono::year{ year },
            std::chrono::month{ static_cast<unsigned>(month) },
            std::chrono::day{ static_cast<unsigned>(day) } };
        if (!date.ok() || hour < 0 || hour > 23 || minute < 0 || minute > 59)
            return std::nullopt;

        return std::chrono::sys_days{ date }
            + std::chrono::hours{ hour }
            + std::chrono::minutes{ minute }
            - EVENT_NOTICE_UTC_OFFSET;
    }

    void DrawBoardTimeWarning(const std::string& text)
    {
        if (!text.empty() && !ParseBoardTime(text))
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "\"YYYY-MM-DD HH:MM\" で書いてください");
    }

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
    }
}

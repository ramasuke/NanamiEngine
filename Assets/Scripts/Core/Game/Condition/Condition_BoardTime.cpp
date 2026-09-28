#include "Condition_BoardTime.h"

#include <cstdio>
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Condition
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
            - BOARD_TIME_UTC_OFFSET;
    }

    void DrawBoardTimeWarning(const std::string& text)
    {
        if (!text.empty() && !ParseBoardTime(text))
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "\"YYYY-MM-DD HH:MM\" で書いてください");
    }
}

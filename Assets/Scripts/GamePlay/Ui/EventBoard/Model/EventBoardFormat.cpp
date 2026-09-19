#include "EventBoardFormat.h"

#include "../../../../../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../Data/EventNotice/Data_EventNotice.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr const char* EVENT_BOARD_WEEKDAYS[] = { "日", "月", "火", "水", "木", "金", "土" };
    }

    std::string FormatEventBoardDate(const std::chrono::sys_seconds time)
    {
        const auto local = time + Asset::EVENT_NOTICE_UTC_OFFSET;
        const auto days  = std::chrono::floor<std::chrono::days>(local);
        const std::chrono::year_month_day date{ days };
        const std::chrono::weekday weekday{ days };

        return std::to_string(static_cast<unsigned>(date.month())) + "/"
            + std::to_string(static_cast<unsigned>(date.day()))
            + "(" + EVENT_BOARD_WEEKDAYS[weekday.c_encoding()] + ")";
    }

    std::string FormatEventBoardDateTime(const std::chrono::sys_seconds time)
    {
        const auto local = time + Asset::EVENT_NOTICE_UTC_OFFSET;
        const auto days  = std::chrono::floor<std::chrono::days>(local);
        const std::chrono::hh_mm_ss timeOfDay{ local - days };

        const auto minutes = timeOfDay.minutes().count();
        return FormatEventBoardDate(time) + " "
            + std::to_string(timeOfDay.hours().count()) + ":"
            + (minutes < 10 ? "0" : "") + std::to_string(minutes);
    }
}

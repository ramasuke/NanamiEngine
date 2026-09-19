#pragma once
#include <chrono>
#include <string>

namespace GamePlay::Ui
{
    /** @brief "10/1(水) 18:00" の形。日本時間で出す */
    [[nodiscard]] std::string FormatEventBoardDateTime(std::chrono::sys_seconds time);
    /** @brief "10/1(水)" の形。日本時間で出す */
    [[nodiscard]] std::string FormatEventBoardDate(std::chrono::sys_seconds time);
}

#pragma once
#include <chrono>
#include <optional>
#include <string>

namespace GameCore::Condition
{
    // NOTE: 掲示板や条件の時刻は日本時間で書き、日本時間で表示する
    constexpr std::chrono::hours BOARD_TIME_UTC_OFFSET{ 9 };

    // NOTE: データに書く "YYYY-MM-DD HH:MM"(日本時間) を読む。書式が崩れていれば nullopt
    [[nodiscard]] std::optional<std::chrono::sys_seconds> ParseBoardTime(const std::string& text);
    // NOTE: 書式が崩れていればインスペクタに赤字で出す
    void DrawBoardTimeWarning(const std::string& text);
}

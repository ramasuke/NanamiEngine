#pragma once
#include <string>

namespace GamePlay::Ui
{
    // NOTE: 1240 -> "1,240 G"
    [[nodiscard]] std::string FormatMoney(int value);
}

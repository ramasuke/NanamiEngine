#include "Ui_MoneyFormat.h"

#include <cstdlib>

namespace GamePlay::Ui
{
    std::string FormatMoney(const int value)
    {
        std::string digits = std::to_string(std::abs(value));
        for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
            digits.insert(static_cast<std::size_t>(i), ",");
        return (value < 0 ? "-" : "") + digits + " G";
    }
}

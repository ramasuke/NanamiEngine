#include "PauseMenuModel.h"

namespace GamePlay::Ui
{
    void PauseMenuModel::MoveSelection(const int delta)
    {
        const auto count = static_cast<int>(PAUSE_MENU_ENTRIES.size());
        const int next = (static_cast<int>(selectedIndex_) + delta) % count;
        selectedIndex_ = static_cast<std::size_t>(next < 0 ? next + count : next);
    }
}

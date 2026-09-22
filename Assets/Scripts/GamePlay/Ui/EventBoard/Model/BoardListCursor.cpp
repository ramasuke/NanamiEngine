#include "BoardListCursor.h"

#include <algorithm>

namespace GamePlay::Ui
{
    BoardListCursor::BoardListCursor(const size_t count, const size_t visibleRowCount)
        : count_(count)
        , visibleRowCount_(visibleRowCount)
    {
    }

    void BoardListCursor::Select(const size_t index)
    {
        if (index >= count_ || index == selectedIndex_)
            return;

        selectedIndex_ = index;
        if (selectedIndex_ < firstVisibleIndex_)
            firstVisibleIndex_ = selectedIndex_;
        else if (visibleRowCount_ > 0 && selectedIndex_ >= firstVisibleIndex_ + visibleRowCount_)
            firstVisibleIndex_ = selectedIndex_ - visibleRowCount_ + 1;

        onSelectionChanged_.OnNext(selectedIndex_);
    }

    void BoardListCursor::Move(const int delta)
    {
        if (count_ == 0)
            return;

        const int last = static_cast<int>(count_) - 1;
        const int next = std::clamp(static_cast<int>(selectedIndex_) + delta, 0, last);
        Select(static_cast<size_t>(next));
    }
}

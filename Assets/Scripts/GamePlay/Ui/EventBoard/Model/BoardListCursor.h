#pragma once
#include <cstddef>

#include "Packages/R4/R4.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板の一覧の選択位置。行は表示窓の分しか作らないので、窓の先頭もここで持つ。
     * 催し・依頼・お知らせの3つの一覧で同じものを使う。
     */
    class BoardListCursor final
    {
    public:
        BoardListCursor(size_t count, size_t visibleRowCount);

        [[nodiscard]] size_t Count            () const { return count_;             }
        [[nodiscard]] size_t SelectedIndex    () const { return selectedIndex_;     }
        [[nodiscard]] size_t FirstVisibleIndex() const { return firstVisibleIndex_; }
        [[nodiscard]] size_t VisibleRowCount  () const { return visibleRowCount_;   }

        void Select(size_t index);
        /** @brief 端で止める */
        void Move(int delta);

        [[nodiscard]] NanamiEngine::R4::Observable<size_t> OnSelectionChanged() const { return onSelectionChanged_.AsObservable(); }

    private:
        size_t count_             = 0;
        size_t visibleRowCount_   = 0;
        size_t selectedIndex_     = 0;
        size_t firstVisibleIndex_ = 0;
        NanamiEngine::R4::Subject<size_t> onSelectionChanged_;
    };
}

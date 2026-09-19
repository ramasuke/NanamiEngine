#pragma once
#include <array>
#include <cstddef>

namespace GamePlay::Ui
{
    enum class PauseMenuEntry : int
    {
        Status,
        Items,
        Quests,
        Controls,
        ReturnToTitle,
        Resume,
    };

    /// 手帳の目次に並べる順
    constexpr std::array PAUSE_MENU_ENTRIES
    {
        PauseMenuEntry::Status,
        PauseMenuEntry::Items,
        PauseMenuEntry::Quests,
        PauseMenuEntry::Controls,
        PauseMenuEntry::ReturnToTitle,
        PauseMenuEntry::Resume,
    };

    /**
     * 冒険者の手帳(＠メニュー)のModel。目次のどの行を指しているかだけを持つ、
     * プレーンなランタイムクラス(アセットでもコンポーネントでもない)。
     */
    class PauseMenuModel final
    {
    public:
        [[nodiscard]] std::size_t    SelectedIndex() const { return selectedIndex_; }
        [[nodiscard]] PauseMenuEntry Selected     () const { return PAUSE_MENU_ENTRIES[selectedIndex_]; }

        void Reset() { selectedIndex_ = 0; }
        /** @brief 端は反対側へ回り込む */
        void MoveSelection(int delta);

    private:
        std::size_t selectedIndex_ = 0;
    };
}

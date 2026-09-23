#pragma once
#include <memory>
#include <string>
#include <vector>

#include "BoardListCursor.h"
#include "Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../Data/Restoration/Data_RestorationFacility.h"

namespace GameCore::PlayerAvatar
{
    class Wallet;
}

namespace GamePlay::Ui
{
    enum class RestorationBoardState
    {
        Open,
        /** 前提の StoryFlag / 施設がまだ。貼り出すが直せない */
        Locked,
        Restored,
    };

    /** @brief 掲示板の「復興」に並べる普請1件 */
    struct RestorationBoardEntry
    {
        std::shared_ptr<Asset::RestorationFacility> facility;
        RestorationBoardState state = RestorationBoardState::Open;
        /** Open のときだけ意味がある。所持金で足りるか */
        bool isAffordable = false;
        std::string costText;
        std::string stateText;
        /** 札の2行目。前提の文言・「直せる」・「お金が足りない」・「直した」 */
        std::string noteText;
    };

    /**
     * 復興の一覧のModel。並びはデータの順のまま(直しても札が動かない)。
     * 状態は StoryProgress と財布から読み、直すたびに全件を読み直す(直した施設が次の前提になるため)。
     * 教官から復興を任される(StoryFlag::RestorationStarted)までは空。
     */
    class RestorationBoardModel final
    {
    public:
        /** @param wallet プレイヤーがいなければ nullptr(所持金 0 として出し、直せない) */
        RestorationBoardModel(
            const std::vector<std::shared_ptr<Asset::RestorationFacility>>& facilities,
            GameCore::PlayerAvatar::Wallet* wallet,
            size_t visibleRowCount);

        [[nodiscard]] const std::vector<RestorationBoardEntry>& Entries() const { return entries_; }
        [[nodiscard]] const RestorationBoardEntry* Selected() const;
        [[nodiscard]] BoardListCursor&       Cursor()       { return cursor_; }
        [[nodiscard]] const BoardListCursor& Cursor() const { return cursor_; }
        [[nodiscard]] int Balance() const;

        /** @brief 選んでいる普請にお金を払って直す。直せなければ何もせず false */
        bool RestoreSelected();
        /** @brief 所持金と StoryProgress から状態を読み直す */
        void Reevaluate();

    private:
        std::vector<RestorationBoardEntry> entries_;
        GameCore::PlayerAvatar::Wallet* wallet_ = nullptr;
        BoardListCursor cursor_;
    };
}

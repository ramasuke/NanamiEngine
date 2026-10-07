#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <string>

#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestJournal.h"
#include "../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestType.h"

namespace GamePlay::Debug
{
    namespace
    {
        using QuestJournal = GameCore::PlayerAvatar::Quest::QuestJournal;

        /** @param avatar 報酬を受け取る手元のアバター。居なければ達成させない */
        void DrawQuest(QuestJournal& journal, const GameCore::PlayerAvatar::QuestType type, GameCore::IPlayerAvatar* avatar)
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            const std::string name     = std::string(GameCore::PlayerAvatar::ToString(type));
            const bool        isTaking = journal.IsTaking(type);
            if (!isTaking || !avatar)
            {
                Widgets::Label(name, isTaking ? "受注中" : journal.CheckCompleted(type) ? "達成済み" : "-");
                return;
            }

            ImGui::PushID(static_cast<int>(type));
            if (Widgets::ButtonRow(name + " (受注中)", { "達成する" }) == 0)
            {
                // NOTE: 報酬は OnRewarded 経由で Wallet に入る。所持金と一緒に保存する
                journal.CompleteQuest(type);
                avatar->SaveStatus();
            }
            ImGui::PopID();
        }

        void DrawQuests()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            auto&      journal = QuestJournal::Instance();
            const auto avatar  = GameCore::PlayerAvatar::Owner();
            Widgets::Note("「達成する」は受注中のクエストを報告したのと同じ (報酬も出る)。達成済みの記録は消せないので、受けていないクエストは済ませられない。");

            if (!avatar)
                Widgets::Note("プレイヤーがいないので達成はできない (報酬の受け取り先がない)。");

            Widgets::Header("クエスト");
            for (const auto type : GameCore::PlayerAvatar::QUEST_TYPE_NAMES)
                DrawQuest(journal, type, avatar.get());

            Widgets::Header("ファイル");
            if (Widgets::Button("保存内容を読み直す"))
                journal.Reload();
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(Quests, "セーブ/クエスト", 12, GamePlay::Debug::DrawQuests)
#endif

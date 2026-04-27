#pragma once
#include <unordered_set>

#include "../PlayerAvatar_QuestType.h"

// Source - https://stackoverflow.com/a/35304501
// Posted by user3080602, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-23, License - CC BY-SA 3.0

namespace std {
    template <> struct hash<GameCore::PlayerAvatar::QuestType> {
        size_t operator() (const GameCore::PlayerAvatar::QuestType &t) const noexcept { return size_t(t); }
    };
}


namespace GameCore::PlayerAvatar::Quest
{
    static constexpr auto COMPLETED_QUEST_SAVE_KEY = "CompletedQuests";
    
    class CompletedQuestGroup final
    {
    public:
        CompletedQuestGroup();

        void Subscribe(const QuestType& completeQuest);
        [[nodiscard]] bool CheckCompleted(const QuestType& quest) const;

    private:
        std::unordered_set<QuestType> completedQuests_;
    };
}

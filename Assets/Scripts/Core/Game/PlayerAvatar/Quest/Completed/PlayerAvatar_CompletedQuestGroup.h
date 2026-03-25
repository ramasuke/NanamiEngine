#pragma once
#include <unordered_set>

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
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
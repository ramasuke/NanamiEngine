#pragma once
#include <memory>

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ITakeableQuest;

    class ICompleteQuestGroup
    {
    public:
        virtual ~ICompleteQuestGroup() = default;
        virtual void CompleteQuest(const QuestType& completeQuest) = 0;
        /** @brief 受注中のその依頼を終える。同じ QuestType を複数受けていても取り違えない */
        virtual void CompleteTakenQuest(const ITakeableQuest& quest);
        [[nodiscard]] virtual bool CheckCompleted(const QuestType& quest) const = 0;
    };
}

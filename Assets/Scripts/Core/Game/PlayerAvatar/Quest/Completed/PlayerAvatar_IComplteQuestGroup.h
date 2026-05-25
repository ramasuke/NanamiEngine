#pragma once
#include <memory>

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar
{
    class QuestBase;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup
    {
    public:
        virtual ~ICompleteQuestGroup() = default;
        virtual void CompleteQuest(const QuestType& completeQuest) = 0;
        [[nodiscard]] virtual bool CheckCompleted(const QuestType& quest) const = 0;
    };
}

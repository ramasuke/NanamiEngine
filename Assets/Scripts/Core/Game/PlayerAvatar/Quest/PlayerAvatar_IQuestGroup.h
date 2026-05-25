#pragma once
#include <memory>

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    class CompletedQuestGroup;
}

namespace GameCore::PlayerAvatar
{
    class QuestBase;
}

namespace GameCore::PlayerAvatar
{
    class IQuestGroup
    {
    public:
        virtual ~IQuestGroup() = default;
        virtual void Subscribe(const std::shared_ptr<QuestBase>& addQuest) = 0;
    };
}

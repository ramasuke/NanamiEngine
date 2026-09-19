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
    class StoryQuestBase;
}

namespace GameCore::PlayerAvatar
{
    class IQuestGroup
    {
    public:
        virtual ~IQuestGroup() = default;
        virtual void Subscribe(const std::shared_ptr<StoryQuestBase>& addQuest) = 0;
        /** @brief 受注中(まだ達成していない)か */
        [[nodiscard]] virtual bool IsTaking(const QuestType& quest) const = 0;
    };
}

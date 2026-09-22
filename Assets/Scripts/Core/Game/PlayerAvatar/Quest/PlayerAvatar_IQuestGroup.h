#pragma once
#include <memory>

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    class CompletedQuestGroup;
    class ITakeableQuest;
}

namespace GameCore::PlayerAvatar
{
    class IQuestGroup
    {
    public:
        virtual ~IQuestGroup() = default;
        /** @brief 職業を問わないクエスト(メインストーリー・依頼)を受ける */
        virtual void Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest) = 0;
        /** @brief 受注中(まだ達成していない)か */
        [[nodiscard]] virtual bool IsTaking(const QuestType& quest) const = 0;
    };
}

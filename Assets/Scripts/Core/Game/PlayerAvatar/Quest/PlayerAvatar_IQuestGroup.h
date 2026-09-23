#pragma once
#include <memory>
#include <vector>

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ITakeableQuest;
}

namespace GameCore::PlayerAvatar
{
    class IQuestGroup
    {
    public:
        virtual ~IQuestGroup() = default;
        /**
         * @brief 職業を問わないクエスト(メインストーリー・依頼)を受ける
         * @return 同じ QuestType を受注中なら受けずに false
         */
        virtual bool Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest) = 0;
        /** @brief 受注中(まだ達成していない)か */
        [[nodiscard]] virtual bool IsTaking(const QuestType& quest) const = 0;
        /** @brief 古いセーブで職業ごとに持っていた職業を問わないクエストを手放す。QuestJournal へ移すため */
        [[nodiscard]] virtual std::vector<std::shared_ptr<Quest::ITakeableQuest>> ReleaseLegacyQuests() { return {}; }
    };
}

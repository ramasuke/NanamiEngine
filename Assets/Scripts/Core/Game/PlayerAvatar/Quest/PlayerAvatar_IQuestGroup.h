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
        // NOTE: 職業を問わないクエスト (メインストーリー・依頼) を受ける。同じ種類を受注中なら受けずに false
        virtual bool Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest) = 0;
        // NOTE: 受注中(まだ達成していない)か
        [[nodiscard]] virtual bool IsTaking(const QuestType& quest) const = 0;
        // NOTE: 古いセーブで職業ごとに持っていた職業を問わないクエストを手放す。共通の帳面へ移し替えるため
        [[nodiscard]] virtual std::vector<std::shared_ptr<Quest::ITakeableQuest>> ReleaseLegacyQuests() { return {}; }
    };
}

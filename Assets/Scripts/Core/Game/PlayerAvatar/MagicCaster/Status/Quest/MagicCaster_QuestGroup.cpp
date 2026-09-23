#include "MagicCaster_QuestGroup.h"

#include "../../../Quest/PlayerAvatar_ITakeableQuest.h"
#include "../../../Quest/PlayerAvatar_QuestJournal.h"
#include "../../../Record/PlayerAvatar_RecordBook.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    QuestGroup::QuestGroup() = default;
    QuestGroup::~QuestGroup() = default;

    bool QuestGroup::Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest)
    {
        return Quest::QuestJournal::Instance().Take(addQuest);
    }

    bool QuestGroup::IsTaking(const QuestType& quest) const
    {
        return Quest::QuestJournal::Instance().IsTaking(quest);
    }

    std::vector<std::shared_ptr<Quest::ITakeableQuest>> QuestGroup::ReleaseLegacyQuests()
    {
        return legacyStoryQuests_.Release();
    }

    void QuestGroup::OnDrawGui()
    {
        Quest::QuestJournal::Instance().OnDrawGui();
        Record::RecordBook::Instance().OnDrawGui();
    }

    void QuestGroup::CompleteQuest(const QuestType& completeQuest)
    {
        Quest::QuestJournal::Instance().CompleteQuest(completeQuest);
    }

    bool QuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return Quest::QuestJournal::Instance().CheckCompleted(quest);
    }
}

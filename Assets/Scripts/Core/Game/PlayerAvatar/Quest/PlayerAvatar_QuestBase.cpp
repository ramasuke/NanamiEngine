
#include "PlayerAvatar_QuestBase.h"

namespace GameCore::PlayerAvatar
{
    QuestBase:: QuestBase() = default;
    QuestBase::~QuestBase() = default;

    void QuestBase::StartQuest(
        const SwordMan::IObservableStatusEvent& event,
        Quest::CompletedQuestGroup& completedQuestGroup)
    {
        DoStartQuest(event);
    }

    void QuestBase::OnDrawGui()
    {
        DoDrawGui();
    }
}

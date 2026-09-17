
#include "PlayerAvatar_QuestBase.h"

namespace GameCore::PlayerAvatar
{
    QuestBase:: QuestBase() = default;
    QuestBase::~QuestBase() = default;

    void QuestBase::StartQuest(const Npc::Friendly::Behaviour::Action::SwordManQuestContext& context)
    {
        DoStartQuest(context);
    }

    void QuestBase::OnDrawGui()
    {
        DrawRewardGui();
        DoDrawGui();
    }
}

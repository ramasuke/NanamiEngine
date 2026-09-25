#include "PlayerAvatar_MainStoryQuestBase.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar
{
    MainStoryQuestBase:: MainStoryQuestBase() = default;
    MainStoryQuestBase::~MainStoryQuestBase() = default;

    void MainStoryQuestBase::StartQuest(const Quest::QuestContext& context)
    {
        DoStartQuest(context);
    }

    void MainStoryQuestBase::OnDrawGui()
    {
        DrawRewardGui();
        DoDrawGui();
    }
}

NANAMI_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Quest::ITakeableQuest, GameCore::PlayerAvatar::MainStoryQuestBase);

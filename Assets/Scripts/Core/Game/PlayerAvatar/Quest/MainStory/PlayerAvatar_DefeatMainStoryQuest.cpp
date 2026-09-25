#include "PlayerAvatar_DefeatMainStoryQuest.h"

#include "../PlayerAvatar_QuestContext.h"
#include "../PlayerAvatar_TakeableQuestFactory.h"
#include "../Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../Record/PlayerAvatar_IRecordBook.h"
#include "../../../Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Quest::MainStory
{
    DefeatMainStoryQuest:: DefeatMainStoryQuest() = default;
    DefeatMainStoryQuest::~DefeatMainStoryQuest()
    {
        subscription_.Dispose();
    }

    void DefeatMainStoryQuest::DoStartQuest(const QuestContext& context)
    {
        subscription_.Dispose();
        auto& completedQuests = context.completedQuests;
        if (Story::StoryProgress::Instance().IsSet(clearedFlag_))
        {
            Complete(completedQuests);
            return;
        }

        const auto kind = enemyKind_;
        subscription_ = context.records.OnDefeat()
            .Where([kind](const Npc::Enemy::EnemyKind defeated) { return defeated == kind; })
            .Subscribe([this, &completedQuests](Npc::Enemy::EnemyKind) { Complete(completedQuests); });
    }

    void DefeatMainStoryQuest::Complete(ICompleteQuestGroup& completedQuests)
    {
        // NOTE: CompleteQuest の中で受注リストから外されて自分が破棄されるので、先に購読を切り、種別は写してから渡す
        subscription_.Dispose();
        const auto type = questType_;
        completedQuests.CompleteQuest(type);
    }

    void DefeatMainStoryQuest::DoDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("questType_", questType_, QUEST_TYPE_NAMES, PlayerAvatar::ToString);
        LibCore::ImGuiHelper::OnDrawEnumField("enemyKind_", enemyKind_, Npc::Enemy::ENEMY_KINDS, Npc::Enemy::ToString);
        LibCore::ImGuiHelper::OnDrawEnumField("clearedFlag_", clearedFlag_, Story::STORY_FLAGS, Story::ToString);
    }

    REGISTER_TAKEABLE_QUEST(DefeatMainStoryQuest)
}

NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::MainStory::DefeatMainStoryQuest, GameCore::PlayerAvatar::MainStoryQuestBase);

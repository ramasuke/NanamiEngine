#pragma once

namespace GameCore::Story
{
    class StoryProgress;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    /** @brief 解放条件が見るもの。どちらも nullptr のことがある */
    struct QuestUnlockContext
    {
        const Story::StoryProgress* story           = nullptr;
        const ICompleteQuestGroup*  completedQuests = nullptr;
    };
}

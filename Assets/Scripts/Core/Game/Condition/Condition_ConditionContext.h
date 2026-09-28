#pragma once
#include <chrono>
#include <optional>

namespace GameCore::Story
{
    class StoryProgress;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

namespace GameCore::Condition
{
    /** @brief 条件が見るもの。どれも空のことがある */
    struct ConditionContext
    {
        const Story::StoryProgress*                     story           = nullptr;
        const PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests = nullptr;
        /** @brief 空なら期間の条件は満たさない */
        std::optional<std::chrono::sys_seconds>         now;
    };
}

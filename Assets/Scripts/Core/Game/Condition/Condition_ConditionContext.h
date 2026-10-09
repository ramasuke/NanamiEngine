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

namespace GameCore::Decoration
{
    class DecorationCollection;
}

namespace GameCore::Navigation
{
    class INavigationSceneSource;
}

namespace GameCore::Condition
{
    // NOTE: 条件が見るContext
    struct ConditionContext
    {
        const Story::StoryProgress&                     story;
        // NOTE: プレイヤーがいなければ nullptr(クエストの条件は満たさない)
        const PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests = nullptr;
        // NOTE: 空なら期間の条件は満たさない
        std::optional<std::chrono::sys_seconds>         now;
        const Decoration::DecorationCollection&         decorations;
        // NOTE: 今のメインシーン。nullptr ならシーンの目標の条件は満たさない
        const Navigation::INavigationSceneSource*       scene = nullptr;
    };
}

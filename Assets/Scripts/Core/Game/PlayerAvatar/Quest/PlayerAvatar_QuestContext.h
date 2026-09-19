#pragma once

namespace GameCore::PlayerAvatar
{
    class IStatusEvent;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;

    /// クエストの実行中に触れてよいプレイヤー側の口。職業を問わないものだけを渡す
    struct QuestContext
    {
        const IStatusEvent&  statusEvent;
        ICompleteQuestGroup& completedQuests;
    };
}

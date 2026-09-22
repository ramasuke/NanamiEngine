#pragma once

namespace GameCore::PlayerAvatar
{
    class IStatusEvent;
}

namespace GameCore::PlayerAvatar::Record
{
    class IRecordBook;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;

    /// クエストの実行中に触れてよいプレイヤー側の口。職業を問わないものだけを渡す
    struct QuestContext
    {
        const IStatusEvent&        statusEvent;
        ICompleteQuestGroup&       completedQuests;
        /** @brief 職業をまたいで1冊の記録帳。討伐・収集の依頼はここの数を見る */
        const Record::IRecordBook& records;
    };
}

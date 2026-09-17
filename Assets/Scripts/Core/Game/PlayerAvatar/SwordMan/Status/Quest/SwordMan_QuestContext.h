#pragma once

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class IObservableStatusEvent;
    class IControlGuideFocusRequest;
}

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    /// クエストの実行中に触れてよいプレイヤー側の口
    struct SwordManQuestContext
    {
        const PlayerAvatar::SwordMan::IObservableStatusEvent& statusEvent;
        PlayerAvatar::SwordMan::IControlGuideFocusRequest&    guideFocus;
        PlayerAvatar::Quest::ICompleteQuestGroup&             completedQuests;
    };
}

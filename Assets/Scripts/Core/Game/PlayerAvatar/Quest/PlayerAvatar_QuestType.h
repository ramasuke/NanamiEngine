#pragma once
#include <string_view>
#include <array>

namespace GameCore::PlayerAvatar
{
    enum class QuestType : int
    {
        SwordManActionInstructTutorial = 0,
        
        Kill10Slimes,
        FindLostRing,
    };

    constexpr std::string_view ToString(const QuestType type)
    {
        switch (type)
        {
        case QuestType::SwordManActionInstructTutorial: return "SwordManActionInstructTutorial";
        case QuestType::Kill10Slimes: return "Kill10Slimes";
        case QuestType::FindLostRing: return "FindLostRing";
        }
        return "UnknownQuestType";
    }

    constexpr std::array QUEST_TYPE_NAMES{
        QuestType::SwordManActionInstructTutorial,
        QuestType::Kill10Slimes,
        QuestType::FindLostRing,
    };
}

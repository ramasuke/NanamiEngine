#pragma once
#include <string_view>
#include <array>

namespace GameCore::PlayerAvatar
{
    // セーブや掲示板データには int で残るので、新しい値は必ず末尾に足す
    enum class QuestType : int
    {
        SwordManActionInstructTutorial = 0,

        Kill10Slimes,
        FindLostRing,

        // 汎用の依頼(討伐・収集)。依頼書ごとに1つ割り当てる
        GrasslandHyenaCull,
        HyenaHuntWeek1,
        RockyTyrant,
        DesertLostCargo,
    };

    constexpr std::string_view ToString(const QuestType type)
    {
        switch (type)
        {
        case QuestType::SwordManActionInstructTutorial: return "SwordManActionInstructTutorial";
        case QuestType::Kill10Slimes: return "Kill10Slimes";
        case QuestType::FindLostRing: return "FindLostRing";
        case QuestType::GrasslandHyenaCull: return "GrasslandHyenaCull";
        case QuestType::HyenaHuntWeek1: return "HyenaHuntWeek1";
        case QuestType::RockyTyrant: return "RockyTyrant";
        case QuestType::DesertLostCargo: return "DesertLostCargo";
        }
        return "UnknownQuestType";
    }

    constexpr std::array QUEST_TYPE_NAMES{
        QuestType::SwordManActionInstructTutorial,
        QuestType::Kill10Slimes,
        QuestType::FindLostRing,
        QuestType::GrasslandHyenaCull,
        QuestType::HyenaHuntWeek1,
        QuestType::RockyTyrant,
        QuestType::DesertLostCargo,
    };
}

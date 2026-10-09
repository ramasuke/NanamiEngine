#pragma once
#include <string_view>
#include <array>

namespace GameCore::PlayerAvatar
{
    // WARNING: セーブや掲示板データには int で残るので、新しい値は必ず末尾に足す
    enum class QuestType : int
    {
        SwordManActionInstructTutorial = 0,

        Kill10Slimes,
        FindLostRing,

        // NOTE: 汎用依頼
        GrasslandHyenaCull,
        HyenaHuntWeek1,
        RockyTyrant,
        DesertLostCargo,

        // NOTE: メインストーリー
        GrassLandTyrant,

        // NOTE: 砂漠
        DesertSkeletonDragon,
        DesertScorpionCull,
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
        case QuestType::GrassLandTyrant: return "GrassLandTyrant";
        case QuestType::DesertSkeletonDragon: return "DesertSkeletonDragon";
        case QuestType::DesertScorpionCull: return "DesertScorpionCull";
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
        QuestType::GrassLandTyrant,
        QuestType::DesertSkeletonDragon,
        QuestType::DesertScorpionCull,
    };
}

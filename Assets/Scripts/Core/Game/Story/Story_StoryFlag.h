#pragma once
#include <array>
#include <string_view>

namespace GameCore::Story
{
    // WARNING: セーブに int で残るので、新しい値は必ず末尾に足す。一覧は docs/Story.md
    enum class StoryFlag : int
    {
        // NOTE: 序章でドラゴンを撃退し、拠点の島に降りた
        PrologueCleared = 0,
        // NOTE: 教官から島の復興を任された
        RestorationStarted,
        // NOTE: 草原の大顎を倒し、緑の浮遊石を取り戻した
        GrassLandCleared,
        // NOTE: 緑の浮遊石が拠点の島の底に戻った(戻ってくる演出を見た)
        GreenStoneReturned,
        // NOTE: 緑の浮遊石の力で、噴水の島と階段が拠点の島の横へ戻ってきた(戻ってくる演出を見た)
        FountainIslandReturned,
        // NOTE: 砂漠の骸竜 (光の浮遊石に起こされた守り竜の亡骸) を倒し、光の浮遊石を取り戻した
        DesertCleared,
        // NOTE: 城塞の手前で座り込んでいた隊商の護衛を見つけ、泉へ帰した
        DesertGuardRescued,
        // NOTE: 光の浮遊石が拠点の島の底に戻った(戻ってくる演出を見た)
        LightStoneReturned,
        // NOTE: 教官から、島が古竜の巣へ引かれていると聞いた (巣へ渡れるようになる)
        NestVoyageStarted,
        // NOTE: 巣で古竜を倒し、積まれていた心臓が空へ散った
        AncientDragonDefeated,
        // NOTE: 古竜を倒した後の話 (エピローグ) を教官から聞いた
        EpilogueHeard,
        
        // NOTE: 草原・砂漠・巣に初めて着いたときの演出を見た
        GrassLandOverviewSeen,
        DesertOverviewSeen,
        DragonNestOverviewSeen,
    };

    constexpr std::string_view ToString(const StoryFlag flag)
    {
        switch (flag)
        {
        case StoryFlag::PrologueCleared:    return "PrologueCleared";
        case StoryFlag::RestorationStarted: return "RestorationStarted";
        case StoryFlag::GrassLandCleared:   return "GrassLandCleared";
        case StoryFlag::GreenStoneReturned: return "GreenStoneReturned";
        case StoryFlag::FountainIslandReturned: return "FountainIslandReturned";
        case StoryFlag::DesertCleared:      return "DesertCleared";
        case StoryFlag::DesertGuardRescued: return "DesertGuardRescued";
        case StoryFlag::LightStoneReturned: return "LightStoneReturned";
        case StoryFlag::NestVoyageStarted:  return "NestVoyageStarted";
        case StoryFlag::AncientDragonDefeated: return "AncientDragonDefeated";
        case StoryFlag::EpilogueHeard:      return "EpilogueHeard";
        case StoryFlag::GrassLandOverviewSeen:  return "GrassLandOverviewSeen";
        case StoryFlag::DesertOverviewSeen:     return "DesertOverviewSeen";
        case StoryFlag::DragonNestOverviewSeen: return "DragonNestOverviewSeen";
        }
        return "UnknownStoryFlag";
    }

    constexpr std::array STORY_FLAGS{
        StoryFlag::PrologueCleared,
        StoryFlag::RestorationStarted,
        StoryFlag::GrassLandCleared,
        StoryFlag::GreenStoneReturned,
        StoryFlag::FountainIslandReturned,
        StoryFlag::DesertCleared,
        StoryFlag::DesertGuardRescued,
        StoryFlag::LightStoneReturned,
        StoryFlag::NestVoyageStarted,
        StoryFlag::AncientDragonDefeated,
        StoryFlag::EpilogueHeard,
        StoryFlag::GrassLandOverviewSeen,
        StoryFlag::DesertOverviewSeen,
        StoryFlag::DragonNestOverviewSeen,
    };
}

#pragma once
#include <array>
#include <string_view>

namespace GameCore::Story
{
    // NOTE: セーブには int で残るので、docs/Story.md
    enum class StoryFlag : int
    {
        // 序章でドラゴンを撃退し、拠点の島に降りた
        PrologueCleared = 0,
        // 教官から島の復興を任された
        RestorationStarted,
        // 草原の大顎を倒し、緑の核片を手に入れた
        GrassLandCleared,
    };

    constexpr std::string_view ToString(const StoryFlag flag)
    {
        switch (flag)
        {
        case StoryFlag::PrologueCleared:    return "PrologueCleared";
        case StoryFlag::RestorationStarted: return "RestorationStarted";
        case StoryFlag::GrassLandCleared:   return "GrassLandCleared";
        }
        return "UnknownStoryFlag";
    }

    constexpr std::array STORY_FLAGS{
        StoryFlag::PrologueCleared,
        StoryFlag::RestorationStarted,
        StoryFlag::GrassLandCleared,
    };
}

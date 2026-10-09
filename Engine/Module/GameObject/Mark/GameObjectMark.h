#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>

struct ImDrawList;
struct ImVec2;

namespace NanamiEngine::Module::GameObject
{
    // NOTE: Unity の Select Icon 相当。ゲーム画面上でオブジェクトの位置に描くマーク
    enum class GameObjectMark : uint8_t
    {
        None = 0,
        LabelGray,   LabelBlue,   LabelTeal,   LabelGreen,   LabelYellow,   LabelOrange,   LabelRed,   LabelPurple,
        CircleGray,  CircleBlue,  CircleTeal,  CircleGreen,  CircleYellow,  CircleOrange,  CircleRed,  CirclePurple,
        DiamondGray, DiamondBlue, DiamondTeal, DiamondGreen, DiamondYellow, DiamondOrange, DiamondRed, DiamondPurple,
        Count
    };

    enum class GameObjectMarkShape : uint8_t
    {
        None,
        Label,
        Circle,
        Diamond
    };

    constexpr int GAMEOBJECT_MARK_COLOR_COUNT = 8;

    static constexpr const char* GAMEOBJECT_MARK_NAMES[] = {
        "None",
        "LabelGray",   "LabelBlue",   "LabelTeal",   "LabelGreen",   "LabelYellow",   "LabelOrange",   "LabelRed",   "LabelPurple",
        "CircleGray",  "CircleBlue",  "CircleTeal",  "CircleGreen",  "CircleYellow",  "CircleOrange",  "CircleRed",  "CirclePurple",
        "DiamondGray", "DiamondBlue", "DiamondTeal", "DiamondGreen", "DiamondYellow", "DiamondOrange", "DiamondRed", "DiamondPurple"
    };

    static_assert(
        static_cast<int>(GameObjectMark::Count) == sizeof(GAMEOBJECT_MARK_NAMES) / sizeof(const char*),
        "GAMEOBJECT_MARK_NAMES count must match GameObjectMark enum count!"
    );

    [[nodiscard]] constexpr GameObjectMark ToMark(const int index)
    {
        if (index < 0 || index >= static_cast<int>(GameObjectMark::Count))
            return GameObjectMark::None;

        return static_cast<GameObjectMark>(index);
    }

    [[nodiscard]] constexpr int ToIndex(const GameObjectMark mark)
    {
        return static_cast<int>(mark);
    }

    [[nodiscard]] constexpr const char* ToName(const GameObjectMark mark)
    {
        // NOTE: ファイルを手で書き換えた等で範囲外の値が入っていても None 扱いにする
        return GAMEOBJECT_MARK_NAMES[ToIndex(ToMark(ToIndex(mark)))];
    }

    [[nodiscard]] constexpr GameObjectMarkShape ToShape(const GameObjectMark mark)
    {
        if (mark == GameObjectMark::None || ToIndex(mark) >= static_cast<int>(GameObjectMark::Count))
            return GameObjectMarkShape::None;

        return static_cast<GameObjectMarkShape>(1 + (ToIndex(mark) - 1) / GAMEOBJECT_MARK_COLOR_COUNT);
    }

    [[nodiscard]] constexpr int ToColorIndex(const GameObjectMark mark)
    {
        if (ToShape(mark) == GameObjectMarkShape::None)
            return 0;

        return (ToIndex(mark) - 1) % GAMEOBJECT_MARK_COLOR_COUNT;
    }

    // NOTE: center を中心にマークを描く。Label のときは label を文字として載せる
    NANAMI_API void DrawMark(ImDrawList& drawList, const ImVec2& center, GameObjectMark mark, const char* label);

    // NOTE: 現在の ImGui 行にマークの小さなプレビューを 1 アイテムとして置く
    NANAMI_API void DrawMarkPreviewGui(GameObjectMark mark);

    // NOTE: 変更があれば true
    NANAMI_API bool DrawChoiceMarkGui(const char* label, GameObjectMark& mark);
}

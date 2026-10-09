#pragma once

namespace GameCore::PlayerAvatar
{
    // NOTE: 演出から指示するキャラ共通のState。各キャラが自分のStateTypeへ翻訳する
    enum class EventSceneStateType
    {
        Idle,
        Walk,
        ArmStretch,
        WarpIn,
        GetUp,
    };
}

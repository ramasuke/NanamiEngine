#pragma once

namespace GameCore::PlayerAvatar
{
    /**
     * @brief 演出(到着ムービー・飛空艇・蘇生RPC)から指示できる、キャラ共通のState。
     * 各キャラのStateMachineが自分のStateTypeへ翻訳する。
     */
    enum class EventSceneStateType
    {
        Idle,
        Walk,
        ArmStretch,
        WarpIn,
        GetUp,
    };
}

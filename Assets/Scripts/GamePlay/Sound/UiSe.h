#pragma once

namespace GamePlay::Sound
{
    // NOTE: UI の効果音
    enum class UiSe
    {
        // NOTE: 系統 A: 手で触れる物 (紙・木・鉄・石)
        Cursor,
        Confirm,
        Cancel,
        Open,
        Close,
        Tab,
        Stamp,
        Refuse,
        Digit,
        GameStart,
        StoneCursor,
        StoneConfirm,
        HoofTick,
        LoadingDone,
        // NOTE: 系統 B: HUD (革袋・布・鉄の留め具・低い空気のうなり)
        HudSelect,
        HudPaletteOpen,
        HudPageShift,
        HudLockOn,
        HudLockOff,
        HudReady,
        HudNotice,
        HudClear,
        HudBossAppear,
        HudInteract,
        // NOTE: 会話
        ChatOpen,
        ChatBlip,
    };
}

#pragma once

namespace GamePlay::Sound
{
    /**
     * @brief UI の効果音。語彙は docs/UIDesign.md の2系統に合わせてある
     * NOTE: 音は tools/art/ui_sfx.py が作る (Assets/Audio/UI/Ui_* / Hud_* / Chat_*)
     */
    enum class UiSe
    {
        // 系統 A: 手で触れる物 (紙・木・鉄・石)
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
        // 系統 B: HUD (革袋・布・鉄の留め具・低い空気のうなり)
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
        // 会話
        ChatOpen,
        ChatBlip,
    };
}

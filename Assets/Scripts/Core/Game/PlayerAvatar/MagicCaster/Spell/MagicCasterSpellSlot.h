#pragma once

namespace GameCore::PlayerAvatar::MagicCaster
{
    /** 1ページの枠数。上=Y(1) 右=B(2) 下=A(3) 左=X(4) の時計回り */
    constexpr int SPELL_SLOTS_PER_PAGE = 4;
    /** 持ち込める枠数。LT で 0〜3、LT+RB で 4〜7 */
    constexpr int SPELL_LOADOUT_SLOT_COUNT = SPELL_SLOTS_PER_PAGE * 2;
    /** RT / 左クリックの基本魔法の枠番号 */
    constexpr int SPELL_BASIC_SLOT = SPELL_LOADOUT_SLOT_COUNT;
    /** クールタイムを管理する枠数（持ち込み + 基本魔法） */
    constexpr int SPELL_SLOT_COUNT = SPELL_LOADOUT_SLOT_COUNT + 1;
}

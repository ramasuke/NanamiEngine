#pragma once

namespace GameCore::PlayerAvatar::MagicCaster
{
    constexpr int SPELL_SLOTS_PER_PAGE = 4;
    constexpr int SPELL_LOADOUT_SLOT_COUNT = SPELL_SLOTS_PER_PAGE * 2;
    constexpr int SPELL_BASIC_SLOT = SPELL_LOADOUT_SLOT_COUNT;
    constexpr int SPELL_SLOT_COUNT = SPELL_LOADOUT_SLOT_COUNT + 1;
    constexpr int SPELL_COUNTER_SLOT = SPELL_SLOT_COUNT;
}

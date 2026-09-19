#pragma once

namespace GameCore::Magic
{
    class IMagicCaster;
    class IMagicSpell;
    struct MagicCastTarget;
}

namespace GamePlay::Magic
{
    /** @brief 撃った人の画面で呼ぶ。狙いを決めて自分の画面で実行し、他の画面へ CastSpellRpc を送る */
    void CastSpell(const GameCore::Magic::IMagicSpell& spell, const GameCore::Magic::IMagicCaster& caster);
    /** @brief 全員の画面で共通の実行。詠唱音もここで鳴らす */
    void ExecuteSpell(const GameCore::Magic::IMagicSpell& spell,
                      const GameCore::Magic::IMagicCaster& caster,
                      const GameCore::Magic::MagicCastTarget& target);
}

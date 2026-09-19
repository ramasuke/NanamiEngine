#pragma once
#include <cstdint>

#include "cereal/cereal.hpp"
#include "MagicCastTarget.h"

namespace GameCore::Magic
{
    class IMagicCaster;

    // 魔法1つが実際に何をするか。MagicSpellData が持ち、中身の値は .magicSpell の .meta に入る
    class IMagicSpellEffect
    {
    public:
        virtual ~IMagicSpellEffect() = default;
        /** @brief 撃った人の画面だけで呼ぶ。ロックオンや地形を見て狙いを決める */
        [[nodiscard]] virtual MagicCastTarget Aim(const IMagicCaster& caster) const = 0;
        /** @brief 全員の画面で呼ぶ。ダメージや回復は対象を持っている画面だけが入れること */
        virtual void Execute(const IMagicCaster& caster, const MagicCastTarget& target) const = 0;
        virtual void OnDrawGui() = 0;

        template<class Archive> void save(Archive& archive, const std::uint32_t version) const {}
        template<class Archive> void load(Archive& archive, const std::uint32_t version) {}
    };
}

CEREAL_CLASS_VERSION(GameCore::Magic::IMagicSpellEffect, 0)

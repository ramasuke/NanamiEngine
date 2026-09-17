#pragma once
#include <array>
#include <string_view>

namespace GameCore::Npc::Enemy
{
    /** EnemyFactory がどのプレハブを生成し、生成後に何を配線するかを選ぶ種別 */
    enum class EnemyKind : int
    {
        NormalBoss = 0,
        Normal = 1,
        Hyena = 2
    };

    constexpr std::array ENEMY_KINDS
    {
        EnemyKind::NormalBoss,
        EnemyKind::Normal,
        EnemyKind::Hyena,
    };

    constexpr std::string_view ToString(const EnemyKind kind)
    {
        switch (kind)
        {
        case EnemyKind::NormalBoss: return "NormalBoss";
        case EnemyKind::Normal: return "Normal";
        case EnemyKind::Hyena: return "Hyena";
        }

        return "Unknown";
    }
}

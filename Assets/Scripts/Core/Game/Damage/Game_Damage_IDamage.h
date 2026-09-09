#pragma once
#include "vec3.hpp"

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace GameCore
{
    struct IDamage
    {
    public:
        virtual ~IDamage() = default;
        virtual int DamageValue() = 0;
        /** @brief 攻撃者から離れる方向(ノックバックに使う正規化ベクトル) */
        [[nodiscard]] virtual glm::vec3 DamageDirection() const = 0;
    };
}

#pragma once
#include <memory>
#include "vec3.hpp"
#include "../../../../../Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
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
        /** @brief 当たった部位のコライダー。本体に当たった場合や部位を持たない攻撃では空 */
        [[nodiscard]] virtual std::weak_ptr<GameObject::IGameObject> HitPart() const = 0;
        /** @brief 溜め攻撃か。露出中の弱点に当てるとスタンを取れる */
        [[nodiscard]] virtual bool IsChargedAttack() const = 0;
    };
}

#pragma once
#include "../../../StatusParameter/Health/Health.h"

namespace GameCore::PlayerAvatar::Item
{
    // アイテムの効果を受ける側。効果クラスはこれだけを見るので、使い手のアバターの種類を知らない
    class IItemEffectTarget
    {
    public:
        virtual ~IItemEffectTarget() = default;
        virtual void Heal(StatusParameter::Health amount) = 0;
        virtual void RestoreStamina(float amount) = 0;
        virtual void ApplyAttackBuff(float rate, float duration_secs) = 0;
    };
}

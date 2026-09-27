#pragma once
#include <string>

#include "vec3.hpp"

namespace GameCore::Npc::Enemy
{
    class IEnemyWarningEffectProvider;
}

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    struct TickContext;

    /**
     * 攻撃の予兆を敵のボーン boneName(ボーン空間の boneOffset)に出す。
     * 権威側限定Tickなら他のピアにも同じものを出させる。provider が nullptr なら出さない
     */
    void FireAttackWarning(
        const TickContext& context,
        const IEnemyWarningEffectProvider* provider,
        const std::string& boneName,
        const glm::vec3& boneOffset);
}

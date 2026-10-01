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
     * 攻撃の予兆を boneName(ボーン空間の boneOffset)に出す。権威側限定Tickなら他ピアにも出させる
     */
    void FireAttackWarning(
        const TickContext& context,
        const IEnemyWarningEffectProvider* provider,
        const std::string& boneName,
        const glm::vec3& boneOffset);
}

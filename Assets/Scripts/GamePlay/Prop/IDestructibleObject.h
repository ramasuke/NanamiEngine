#pragma once
#include "../../Core/Game/Npc/Enemy/ITakableEnemyAttack/ITakableEnemyAttack.h"
#include "../../Core/Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"

namespace GamePlay::Prop
{
    class IDestructibleObject : public GameCore::PlayerAvatar::ITakablePlayerAttack,
                                public GameCore::Npc::Enemy::ITakableEnemyAttack
    {
    public:
        virtual ~IDestructibleObject() = default;
    };
}

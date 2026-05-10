#pragma once
#include <memory>

namespace GameCore
{
    struct IDamage;
}


namespace GameCore::Npc::Enemy
{
    class ITakableEnemyAttack
    {
    public:
        virtual ~ITakableEnemyAttack() = default;
        virtual void OnTakeDamage(std::unique_ptr<IDamage> context) = 0;
    };
};

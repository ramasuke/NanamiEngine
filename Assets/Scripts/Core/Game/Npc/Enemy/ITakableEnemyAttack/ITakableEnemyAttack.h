#pragma once
#include <memory>

namespace GameCore
{
    struct IDamageContext;
}


namespace GameCore::Npc::Enemy
{
    class ITakableEnemyAttack
    {
    public:
        virtual ~ITakableEnemyAttack() = default;
        virtual void OnTakeDamage(std::unique_ptr<IDamageContext> context) = 0;
    };
};

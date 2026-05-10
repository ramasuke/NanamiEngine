#pragma once

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
    };
}

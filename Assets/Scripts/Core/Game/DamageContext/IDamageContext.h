#pragma once

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace GameCore
{
    struct IDamageContext
    {
    public:
        virtual ~IDamageContext() = default;
        virtual int DamageValue() = 0;
    };
}

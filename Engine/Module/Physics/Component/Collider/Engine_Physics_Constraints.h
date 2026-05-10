#pragma once
#include <cstdint>

namespace NanamiEngine::Module::Physics
{
    enum class Constraints : uint8_t
    {
        None = 0,
        FreezePosX = 1 << 0,
        FreezePosY = 1 << 1,
        FreezePosZ = 1 << 2,
        FreezeRotX = 1 << 3,
        FreezeRotY = 1 << 4,
        FreezeRotZ = 1 << 5,
    };
    
    inline Constraints operator|(Constraints a, Constraints b)
    {
        return static_cast<Constraints>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline Constraints& operator|=(Constraints& a, Constraints b)
    {
        a = a | b;
        return a;
    }

    inline bool HasConstraint(Constraints flags, Constraints target)
    {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(target)) != 0;
    }
    
    void DrawConstraintCheckBoxsGui(Constraints& constraints);
}

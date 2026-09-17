#pragma once
#include <cstdint>

namespace NanamiEngine::Module::Physics
{
    // JPH::EMotionType と同じ値・同じ幅にしている。シリアライズ済みデータ(int)をそのまま読めるようにするため
    enum class MotionType : uint8_t
    {
        Static,
        Kinematic,
        Dynamic,
    };

    bool DrawChoiceMotionTypeGui(const char* label, MotionType& motionType);
}

#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>

namespace NanamiEngine::Module::Physics
{
    // NOTE: JPH::EMotionType と同じ値・同じ幅にする。シリアライズ済みの int をそのまま読めるようにするため
    enum class MotionType : uint8_t
    {
        Static,
        Kinematic,
        Dynamic,
    };

    NANAMI_API bool DrawChoiceMotionTypeGui(const char* label, MotionType& motionType);
}

#pragma once
#include <array>
#include <string_view>

namespace GameCore::Magic
{
    // 詠唱モーション。値は MagicCasterAnimation.animTree の CastMotion パラメータの値そのもの
    enum class MagicCastMotion : int
    {
        OneHandThrust    = 0,
        OneHandSweep     = 1,
        OneHandUppercut  = 2,
        OneHandRaise     = 3,
        TwoHandRaise     = 4,
        TwoHandSlam      = 5,
        TwoHandBurst     = 6,
        TwoHandThrow     = 7,
        TwoHandSwingPush = 8,
        TwoHandBeam      = 9,
        TwoHandPushHold  = 10,
        TwoHandPray      = 11,
    };

    constexpr std::array MAGIC_CAST_MOTIONS
    {
        MagicCastMotion::OneHandThrust,
        MagicCastMotion::OneHandSweep,
        MagicCastMotion::OneHandUppercut,
        MagicCastMotion::OneHandRaise,
        MagicCastMotion::TwoHandRaise,
        MagicCastMotion::TwoHandSlam,
        MagicCastMotion::TwoHandBurst,
        MagicCastMotion::TwoHandThrow,
        MagicCastMotion::TwoHandSwingPush,
        MagicCastMotion::TwoHandBeam,
        MagicCastMotion::TwoHandPushHold,
        MagicCastMotion::TwoHandPray,
    };

    constexpr std::string_view ToString(const MagicCastMotion motion)
    {
        switch (motion)
        {
        case MagicCastMotion::OneHandThrust:    return "OneHandThrust";
        case MagicCastMotion::OneHandSweep:     return "OneHandSweep";
        case MagicCastMotion::OneHandUppercut:  return "OneHandUppercut";
        case MagicCastMotion::OneHandRaise:     return "OneHandRaise";
        case MagicCastMotion::TwoHandRaise:     return "TwoHandRaise";
        case MagicCastMotion::TwoHandSlam:      return "TwoHandSlam";
        case MagicCastMotion::TwoHandBurst:     return "TwoHandBurst";
        case MagicCastMotion::TwoHandThrow:     return "TwoHandThrow";
        case MagicCastMotion::TwoHandSwingPush: return "TwoHandSwingPush";
        case MagicCastMotion::TwoHandBeam:      return "TwoHandBeam";
        case MagicCastMotion::TwoHandPushHold:  return "TwoHandPushHold";
        case MagicCastMotion::TwoHandPray:      return "TwoHandPray";
        }
        return "Unknown";
    }
}

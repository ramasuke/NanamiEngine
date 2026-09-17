#pragma once

namespace GameCore::PlayerAvatar::SwordMan
{
    enum class AnimationType : int
    {
        Idle = 0,
        Walk = 1,
        Run  = 2,
        Jump = 3,
        ComboAttack     = 4,
        DashAttack      = 5,
        AvoidRolling    = 6,
        InjuredWalk     = 7,
        InjuredRun      = 8,
        ChargeAttackCharging = 9,
        ChargeAttackRelease  = 10,
        ClimbToTop      = 11,
        AttackedShocked = 12,
        JumpAttackAir   = 13,
        JumpAttackLand  = 14,
        Hurt            = 31,
        Chatting     = 20,
        FallDown     = 40,
        Down         = 41,
        GetUp        = 42,
        ArmStretch   = 101,
        Death        = 300,
    };
}
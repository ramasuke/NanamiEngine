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
        ClimbToTop      = 11,
        AttackedShocked = 12,
        Hurt            = 31,
        OnEnableReinforce  = 50,
        OnDisableReinforce = 51,
        Chatting     = 20,
        ArmStretch   = 101,
        Death        = 300,
    };
}
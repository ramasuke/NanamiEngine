#pragma once

namespace GameCore::Npc::Enemy::FirstEventDragon
{
    enum class AnimationType : int
    {
        Idle            = 0,
        TurnLeft        = 1,
        TurnRight       = 2,
        Walk            = 3,
        FlyingIdle      = 4,
        FlyingToLanding = 5,
        
        Attack1            = 10,
        Attack2            = 11,
        LandingTackle      = 12,
        WeggingTail        = 13,
        RoarSpittingFrames = 14,
        
        LandingRoar        = 30,
    };
}

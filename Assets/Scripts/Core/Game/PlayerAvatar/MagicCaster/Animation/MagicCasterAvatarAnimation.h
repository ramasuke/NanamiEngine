#pragma once

namespace GameCore::PlayerAvatar::MagicCaster
{
    enum class AnimationType : int
    {
        Idle = 0,
        Walk = 1,
        Run  = 2,
        Jump = 3,
        Cast = 4,
        Hurt = 5,
        Death = 6,
    };
}

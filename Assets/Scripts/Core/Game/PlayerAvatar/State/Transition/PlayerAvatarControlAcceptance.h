#pragma once
#include <cstdint>

namespace GameCore::PlayerAvatar
{
    enum class PlayerAvatarControlAcceptance : uint8_t
    {
        None,
        /// 一瞬で終わるので、受け付ける操作は直前の State のものとみなす
        Momentary,
        Accept,
    };
}

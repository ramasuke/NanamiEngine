#pragma once
#include <cstdint>

namespace NanamiEngine::Core::Network
{
    /** 所有者が離脱したときにそのオブジェクトをどう扱うか */
    enum class OwnerLeavePolicy : uint8_t
    {
        Transfer = 0, // 所有権をホストへ移す
        Destroy  = 1, // 破棄する
    };
}

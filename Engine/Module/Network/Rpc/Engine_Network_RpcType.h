#pragma once
#include <cstdint>

#include "Engine_Network_Rpc.h"

namespace NanamiEngine::Module::Network
{
    // エンジン由来の RPC は 0 起点で追加する (ゲーム側は 1,000,000 以降を使う)
    enum class EEngineRpcType : uint32_t
    {
    };
}

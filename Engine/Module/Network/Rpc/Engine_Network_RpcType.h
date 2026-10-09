#pragma once
#include <cstdint>

#include "Engine_Network_Rpc.h"

namespace NanamiEngine::Module::Network
{
    // NOTE: エンジン由来の RPC は 0 起点で追加する。ゲーム側の ID 空間とは重ならないよう分けてある
    enum class EEngineRpcType : uint32_t
    {
    };
}

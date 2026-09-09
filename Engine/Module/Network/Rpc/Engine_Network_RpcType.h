#pragma once
#include <cstdint>

#include "Engine_Network_Rpc.h"

namespace NanamiEngine::Module::Network
{
    // Assets/Scripts側 GameCore::Network::ERpcType とのRpcId数値衝突を避けるため、
    // engine由来のRPCはこの列挙に0起点で追加する。ゲーム側は1,000,000以降を使う規約とする。
    // (例: enum class EEngineRpcType : uint32_t { SyncAnimation, ... };
    //      using XxxRpc = RpcDef<EEngineRpcType::Xxx, Args...>;)
    enum class EEngineRpcType : uint32_t
    {
    };
}

#pragma once

namespace NanamiEngine::Core::Network
{
    enum class ConnectionState
    {
        Connecting,   // クライアントがホストへ接続中(PlayerId 未割り当て)
        Connected,    // PlayerId が割り当て済み
        Failed,       // 待ち受け・接続に失敗した
        Disconnected  // 接続できた後にホストを失った
    };
}

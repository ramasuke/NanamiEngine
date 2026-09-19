#pragma once
#include <cstdint>
#include <string>

namespace NanamiEngine::Core::Network
{
    enum class Mode
    {
        Server,
        Client
    };

    enum class ServerType
    {
        Relay,        // 受信パケットをそのまま全クライアントへブロードキャスト
        Authoritative // サーバーで処理し、選択的にブロードキャスト（OnServerReceive override で制御）
    };

    enum class ConnectionState
    {
        Connecting,   // クライアントがホストへ接続中(PlayerId 未割り当て)
        Connected,    // PlayerId が割り当て済み
        Failed,       // 待ち受け・接続に失敗した
        Disconnected  // 接続できた後にホストを失った
    };

    struct HostEndpoint
    {
        std::string   address;
        std::uint16_t port = 0;
    };

    struct NetworkStartSettings
    {
        Mode         mode = Mode::Client;
        HostEndpoint host; // Client のときだけ使う
    };
}

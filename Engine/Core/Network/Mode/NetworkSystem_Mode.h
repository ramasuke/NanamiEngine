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

    enum class Transport
    {
        Direct,     // ホストが待ち受け、クライアントが直接つなぐ(LAN)
        RelayServer // 双方が中継サーバーへつなぎ、ホストかどうかは中継サーバーが決める
    };

    struct NetworkStartSettings
    {
        Mode         mode = Mode::Client; // Direct のときだけ使う
        HostEndpoint host;                // Direct の Client では接続先、RelayServer では中継サーバー
        Transport    transport = Transport::Direct;
        std::string  sessionKey;          // RelayServer のときだけ使う
    };
}

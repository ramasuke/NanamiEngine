#pragma once

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

    enum class ConnectionTarget
    {
        Localhost,
        LAN
    };
}
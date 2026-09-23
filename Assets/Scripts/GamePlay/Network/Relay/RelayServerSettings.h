#pragma once
#include <cstdint>
#include <string>

#include "cereal/cereal.hpp"
#include "Engine/Core/Network/Mode/NetworkSystem_Mode.h"

namespace GamePlay::Network
{
    /**
     * 中継サーバー(EnviroHunter-Server)の接続情報。LocalPrefs/Network/RelayServer.json に保存し、
     * エディタでは LocalPrefs ウィンドウから編集できる。ファイルが無ければ既定値(本番サーバー)を使う
     */
    struct RelayServerSettings final
    {
        bool          useRelayServer = true;
        std::string   address        = "160.16.76.201";
        std::uint16_t port           = 1234;
        // 中継サーバー上で他のゲームと部屋を分けるための名前
        std::string   appId          = "NanamiEngine";

        /** LocalPrefs から読み直す。ファイルが無い・壊れているときは既定値 */
        [[nodiscard]] static RelayServerSettings Load();

        /** 中継サーバーを使う設定で、接続先と appId が埋まっているか */
        [[nodiscard]] bool IsEnabled() const;
        [[nodiscard]] NanamiEngine::Core::Network::HostEndpoint Endpoint() const { return { address, port }; }

        // DrawLocalPrefWidget から呼ばれる
        void OnDrawGui();

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(
                CEREAL_NVP(useRelayServer),
                CEREAL_NVP(address),
                CEREAL_NVP(port),
                CEREAL_NVP(appId)
            );
        }
    };
}

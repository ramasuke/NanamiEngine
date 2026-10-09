#pragma once
#include <cstdint>
#include <string>

#include "cereal/cereal.hpp"
#include "Engine/Core/Network/Mode/NetworkSystem_HostEndpoint.h"

namespace GamePlay::Network
{
    // NOTE: 中継サーバーの接続情報
    struct RelayServerSettings final
    {
        bool          useRelayServer = true;
        std::string   address        = "160.16.76.201";
        std::uint16_t port           = 1234;
        std::string   appId          = "NanamiEngine";

        [[nodiscard]] static RelayServerSettings Load();

        // NOTE: 中継サーバーを使う設定で、接続先と appId が埋まっているか
        [[nodiscard]] bool IsEnabled() const;
        [[nodiscard]] NanamiEngine::Core::Network::HostEndpoint Endpoint() const { return { address, port }; }

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

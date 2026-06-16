#pragma once
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "Module/SpawnPlayer/Packet_Dispatch_SpawnPlayer.h"

namespace NanamiEngine::Core::Network
{
    class DefaultPacketDispatcher;
}

namespace NanamiEngine::Core::Network
{
    struct Packet;
}

namespace NanamiEngine::Core::Network
{
    class IPlayerIdProvider;
}

namespace NanamiEngine::Core::Network
{
    class IPacketSender;
}

namespace GameCore::Network
{
    class CustomDispatcherGroup final
    {
    public:
        explicit CustomDispatcherGroup(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            Core::Network::IPacketSender& packetSender,
            const Core::Network::IPlayerIdProvider& playerIdProvider,
            Asset::PlayerAvatarFactory& playerAvatarFactory,
            const PlayerAvatar::AllPlayerCameraGroup& cameraGroup);

        void DispatchReceivedPacket(const Core::Network::Packet& packet);

        [[nodiscard]] SpawnPlayerDispatcher& SpawnPlayer() { return spawnPlayerDispatcher_; }

    private:
        SpawnPlayerDispatcher spawnPlayerDispatcher_;
    };
}

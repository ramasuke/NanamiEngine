#pragma once
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "Module/SpawnPlayer/Packet_Dispatch_SpawnPlayer.h"
#include "Module/SpawnEnemy/Packet_Dispatch_SpawnEnemy.h"

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
            Asset::PlayerAvatarFactory& playerAvatarFactory);

        void DispatchReceivedPacket(const Core::Network::Packet& packet);

        [[nodiscard]] SpawnPlayerDispatcher&       SpawnPlayer()       { return spawnPlayerDispatcher_; }
        [[nodiscard]] EnemySpawnDispatcher&        SpawnEnemy()        { return enemySpawnDispatcher_; }

    private:
        SpawnPlayerDispatcher      spawnPlayerDispatcher_;
        EnemySpawnDispatcher       enemySpawnDispatcher_;
    };
}

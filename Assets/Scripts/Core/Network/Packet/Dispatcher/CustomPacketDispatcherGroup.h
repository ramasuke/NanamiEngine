#pragma once
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "Module/SpawnPlayer/Packet_Dispatch_SpawnPlayer.h"
#include "Module/SyncAvatarState/Packet_Dispatch_SyncAvatarState.h"
#include "Module/SyncBehaviourTree/Packet_Dispatch_SyncBehaviourTree.h"

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
        [[nodiscard]] SyncAvatarStateDispatcher&   SyncAvatarState()   { return syncAvatarStateDispatcher_; }
        [[nodiscard]] SyncBehaviourTreeDispatcher& SyncBehaviourTree() { return syncBehaviourTreeDispatcher_; }

    private:
        SpawnPlayerDispatcher      spawnPlayerDispatcher_;
        SyncAvatarStateDispatcher  syncAvatarStateDispatcher_;
        SyncBehaviourTreeDispatcher syncBehaviourTreeDispatcher_;
    };
}

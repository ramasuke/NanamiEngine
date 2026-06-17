#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Module::AnimationTree
{
    struct AnimationStateSnapshot;
}

namespace NanamiEngine::Core::Network
{
    class INetworkObjectInstanceRegistry;

    class SyncAnimationDispatcher final : public PacketDispatcherBase
    {
    public:
        explicit SyncAnimationDispatcher(
            const IPlayerIdProvider& playerIdProvider,
            IPacketSender& packetSender,
            INetworkObjectInstanceRegistry& instanceRegistry)
            : PacketDispatcherBase(playerIdProvider, packetSender)
            , instanceRegistry_(instanceRegistry) {}

        void DispatchSendPacket(
            NetworkObjectId id,
            const Module::AnimationTree::AnimationStateSnapshot& state);

    protected:
        void OnReceive(const Packet& packet) override;

    private:
        INetworkObjectInstanceRegistry& instanceRegistry_;
    };
}

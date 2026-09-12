#pragma once
#include "Module/AssignPlayerId/Packet_Dispatch_AssignPlayerId.h"
#include "Module/SpawnNetworkObject/Packet_Dispatch_SpawnNetworkObject.h"
#include "Module/SyncTransform/Packet_Dispatch_SyncTransform.h"
#include "Module/SyncAnimation/Packet_Dispatch_SyncAnimation.h"
#include "../../Object/Registry/INetworkObjectInstanceRegistry.h"
#include "Module/SyncParameter/Packet_Dispatch_SyncParameter.h"
#include "Module/Rpc/Packet_Dispatch_Rpc.h"
#include "Module/Session/Packet_Dispatch_Session.h"

namespace NanamiEngine::Core::Network
{
    class DefaultPacketDispatcher final
    {
    public:
        explicit DefaultPacketDispatcher(
            INetworkSystem& networkSystem,
            INetworkObjectInstanceRegistry& instanceRegistry);

        [[nodiscard]] SpawnNetworkObject&      Spawn()          { return spawnNetworkObject_; }
        [[nodiscard]] ReceivedAssignPlayerId&  ReceivedAssignPlayerId() { return receivedAssignPlayerId_; }
        [[nodiscard]] SyncTransformDispatcher& SyncTransform() { return syncTransform_; }
        [[nodiscard]] SyncAnimationDispatcher& SyncAnimation() { return syncAnimation_; }
        [[nodiscard]] SyncParameterDispatcher& SyncParameter() { return syncParameter_; }
        [[nodiscard]] RpcDispatcher&           Rpc()           { return rpcDispatcher_; }
        [[nodiscard]] SessionDispatcher&       Session()       { return sessionDispatcher_; }

        [[nodiscard]] std::weak_ptr<Module::GameObject::IGameObject> FindNetworkObject(NetworkObjectId id) const;

        void DispatchReceivedPacket(const Packet& packet);
        void Update();

    private:
        INetworkObjectInstanceRegistry& instanceRegistry_;
        Network::ReceivedAssignPlayerId receivedAssignPlayerId_;
        SpawnNetworkObject spawnNetworkObject_;
        SyncTransformDispatcher syncTransform_;
        SyncAnimationDispatcher syncAnimation_;
        SyncParameterDispatcher syncParameter_;
        RpcDispatcher rpcDispatcher_;
        SessionDispatcher sessionDispatcher_; // syncTransform_ を参照するので後ろに置く
    };
}

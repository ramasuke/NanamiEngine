#include "Packet_Dispatch_SyncAnimation.h"

#include "../../../../Object/Registry/INetworkObjectInstanceRegistry.h"
#include "../../../../../../Module/AnimationTree/AnimationTree.h"
#include "../../../../../../Module/Component/Animator/Animator.h"
#include "../../../../../../Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../../Module/GameObject/Interface/IGameObject.h"

namespace NanamiEngine::Core::Network
{
    void SyncAnimationDispatcher::DispatchSendPacket(
        const NetworkObjectId id,
        const AnimationTree::AnimationStateSnapshot& state)
    {
        Packet packet = Packet::Create(DefaultPacketType::SyncAnimation);
        packet.Data().Write(id);
        packet.Data().Write(state);
        packet.SetDelivery(DeliveryMode::Unreliable);
        SendPacket(packet);
    }

    void SyncAnimationDispatcher::OnReceive(const Packet& packet)
    {
        size_t offset = 0;
        const auto networkObjectId = packet.Data().Read<NetworkObjectId>(offset);

        if (networkObjectId.IsOwnerBy(PlayerId()))
            return;

        const auto state = packet.Data().Read<Module::AnimationTree::AnimationStateSnapshot>(offset);

        const auto gameObject = instanceRegistry_.Find(networkObjectId).lock();
        if (!gameObject)
            return;

        const auto animator = gameObject->Components().Catch<Module::Component::Animator>().lock();
        if (!animator)
            return;

        auto* tree = animator->GetAnimationTree();
        if (!tree)
            return;

        tree->ApplyRemoteState(state, animator->AnimationModelHandle());
    }
}

#include "Packet_Dispatch_SyncAvatarState.h"

#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Game/PlayerAvatar/IPlayerAvatar.h"

namespace GameCore::Network
{
    void SyncAvatarStateDispatcher::DispatchSendPacket(
        const Core::Network::NetworkObjectId id, const uint8_t stateValue)
    {
        Core::Network::Packet packet = Core::Network::Packet::Create(
            static_cast<Core::Network::PacketType>(EPacketType::SyncAvatarState));
        packet.Data().Write(id);
        packet.Data().Write(stateValue);
        packet.SetDelivery(Core::Network::DeliveryMode::Unreliable);
        SendPacket(packet);
    }

    void SyncAvatarStateDispatcher::OnReceive(const Core::Network::Packet& packet)
    {
        size_t offset = 0;
        const auto id         = packet.Data().Read<Core::Network::NetworkObjectId>(offset);
        const auto stateValue = packet.Data().Read<uint8_t>(offset);

        if (id.IsOwnerBy(PlayerId()))
            return;

        auto networkObjectWeak = DefaultDispatch().FindNetworkObject(id);
        auto networkObject = networkObjectWeak.lock();
        if (!networkObject)
            return;
        
        auto& components = networkObject->Components();
        auto avatarWeak = components.Catch<IPlayerAvatar>(); 
        auto avatar = avatarWeak.lock();
        avatar->ApplySyncState(stateValue);
    }
}

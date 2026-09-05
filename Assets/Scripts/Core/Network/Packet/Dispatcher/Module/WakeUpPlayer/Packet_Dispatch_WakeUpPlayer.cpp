#include "Packet_Dispatch_WakeUpPlayer.h"

#include "../../../Custom_PacketType.h"
#include "../../../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../../Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"

namespace GameCore::Network
{
    void WakeUpPlayerDispatcher::DispatchSendPacket(const Core::Network::NetworkObjectId targetId)
    {
        Core::Network::Packet packet = Core::Network::Packet::Create(
            static_cast<Core::Network::PacketType>(EPacketType::WakeUpPlayer));
        packet.Data().Write(targetId);
        packet.SetDelivery(Core::Network::DeliveryMode::Reliable);
        SendPacket(packet);
    }

    void WakeUpPlayerDispatcher::OnReceive(const Core::Network::Packet& packet)
    {
        size_t offset = 0;
        const auto targetId = packet.Data().Read<Core::Network::NetworkObjectId>(offset);

        if (!targetId.IsOwnerBy(PlayerId()))
            return;

        auto networkObjectWeak = DefaultDispatch().FindNetworkObject(targetId);
        auto networkObject = networkObjectWeak.lock();
        if (!networkObject)
            return;

        auto avatarWeak = networkObject->Components().Catch<IPlayerAvatar>();
        auto avatar = avatarWeak.lock();
        if (!avatar)
            return;

        if (!avatar->PlayerStatus().IsDowned())
            return;

        avatar->PlayerStatus().Revive();
        avatar->GetEventSceneStateMachine().OnChangeState(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateType::Idle);
    }
}

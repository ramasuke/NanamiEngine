#include "Game_CustomNetworkRunner.h"

#include "../../../Engine/Core/Network/EnetUDPNetworkSystem.h"

namespace GamePlay::Network
{
    GameCore::Network::CustomDispatcherGroup& CustomNetworkRunner::CustomDispatcher()
    {
        assert(customDispatcherGroup_, "customPacketDispatcher is null");
        return customDispatcherGroup_.value();
    }

    void CustomNetworkRunner::DoInitialize()
    {
        customDispatcherGroup_.emplace(DefaultDispatcher(), PacketSender(), PlayerIdProvider());
    }

    void CustomNetworkRunner::DoDispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        customDispatcherGroup_->DispatchReceivedPacket(packet);
    }

    std::unique_ptr<Core::Network::INetworkSystem> CustomNetworkRunner::DoCreateUseNetworkSystem() const
    {
        return std::make_unique<Core::Network::EnetUDPNetworkSystem>();
    }

    std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> CustomNetworkRunner::SpawnPlayerAvatar(
        Module::Asset::PrefabGameObjectFile& prefabFile,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        return customDispatcherGroup_->SpawnPlayer().DispatchSendPacket(prefabFile, position, rotation);
    }

    void CustomNetworkRunner::OnDrawGui()
    {

    }
}

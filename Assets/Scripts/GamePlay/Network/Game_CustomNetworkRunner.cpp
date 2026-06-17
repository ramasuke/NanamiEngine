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
        customDispatcherGroup_.emplace(
            DefaultDispatcher(),
            PacketSender(),
            PlayerIdProvider(),
            *playerAvatarFactory_.get());
    }

    void CustomNetworkRunner::DoDispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        customDispatcherGroup_->DispatchReceivedPacket(packet);
    }

    std::unique_ptr<Core::Network::INetworkSystem> CustomNetworkRunner::DoCreateUseNetworkSystem() const
    {
        return std::make_unique<Core::Network::EnetUDPNetworkSystem>();
    }

    std::weak_ptr<GameCore::IPlayerAvatar> CustomNetworkRunner::SpawnPlayerAvatar(
        const GameCore::PlayerAvatar::PlayerAvatarType type,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        return customDispatcherGroup_->SpawnPlayer().DispatchSendPacket(
            type,
            position,
            rotation);
    }

    void CustomNetworkRunner::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("playerAvatarFactory_", playerAvatarFactory_);
    }
}

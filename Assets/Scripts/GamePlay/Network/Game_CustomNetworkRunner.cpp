#include "Game_CustomNetworkRunner.h"

#include "Engine/Core/Network/EnetUDPNetworkSystem.h"
#include "Engine/Core/Network/RelayNetworkSystem.h"

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
            *playerAvatarFactory_.get(),
            *enemyFactory_.get());
    }

    void CustomNetworkRunner::DoShutdown()
    {
        customDispatcherGroup_.reset();
    }

    void CustomNetworkRunner::DoDispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        customDispatcherGroup_->DispatchReceivedPacket(packet);
    }

    std::unique_ptr<Core::Network::INetworkSystem> CustomNetworkRunner::DoCreateUseNetworkSystem(
        const Core::Network::NetworkStartSettings& settings) const
    {
        if (settings.transport == Core::Network::Transport::RelayServer)
            return std::make_unique<Core::Network::RelayNetworkSystem>(settings);
        return std::make_unique<Core::Network::EnetUDPNetworkSystem>(settings);
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

    std::shared_ptr<Module::GameObject::IGameObject> CustomNetworkRunner::SpawnEnemy(
        const GameCore::Npc::Enemy::EnemyKind kind,
        const glm::vec3 position,
        const glm::quat rotation)
    {
        return customDispatcherGroup_->SpawnEnemy().DispatchSendPacket(
            kind,
            position,
            rotation);
    }

    void CustomNetworkRunner::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("playerAvatarFactory_", playerAvatarFactory_);
        ImGuiHelper::OnDrawInputField("enemyFactory_", enemyFactory_);
    }
}

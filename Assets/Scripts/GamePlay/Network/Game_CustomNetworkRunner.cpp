#include "Game_CustomNetworkRunner.h"

#include "Engine/Core/Network/EnetUDPNetworkSystem.h"
#include "Relay/EnetRelayNetworkSystem.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Network
{
    GameCore::Network::CustomDispatcherGroup& CustomNetworkRunner::CustomDispatcher()
    {
        assert(customDispatcherGroup_, "customPacketDispatcher is null");
        return customDispatcherGroup_.value();
    }

    void CustomNetworkRunner::StartRelay(const std::string& sessionKey, const RelayServerSettings& relay, const RelayRoom& room)
    {
        pendingRelayStart_ = RelayStart{ sessionKey, relay, room, std::make_shared<RelayRoomStatus>() };
        Start({});
        activeRelay_ = std::move(pendingRelayStart_);
        pendingRelayStart_.reset();
    }

    std::string CustomNetworkRunner::RelayRoomCode() const
    {
        return activeRelay_ ? activeRelay_->status->code : std::string();
    }

    std::optional<std::string> CustomNetworkRunner::RelayFailure() const
    {
        return activeRelay_ ? activeRelay_->status->failure : std::nullopt;
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
        activeRelay_.reset();
    }

    void CustomNetworkRunner::DoDispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        customDispatcherGroup_->DispatchReceivedPacket(packet);
    }

    std::unique_ptr<Core::Network::INetworkSystem> CustomNetworkRunner::DoCreateUseNetworkSystem(
        const Core::Network::NetworkStartSettings& settings) const
    {
        if (pendingRelayStart_)
            return std::make_unique<EnetRelayNetworkSystem>(pendingRelayStart_->settings, pendingRelayStart_->sessionKey,
                                                            pendingRelayStart_->room, pendingRelayStart_->status);
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

        ImGui::Separator();
        if (activeRelay_)
        {
            ImGui::Text("transport: relay %s:%u (app %s, session %s)", activeRelay_->settings.address.c_str(),
                        activeRelay_->settings.port, activeRelay_->settings.appId.c_str(), activeRelay_->sessionKey.c_str());
            if (!activeRelay_->status->code.empty())
                ImGui::Text("room code: %s", activeRelay_->status->code.c_str());
        }
        else
        {
            ImGui::Text("transport: %s", IsStarted() ? "LAN" : "-");
        }
        ImGui::TextDisabled("Relay server settings: toolbar > LocalPrefs > RelayServer");
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Network::CustomNetworkRunner);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Network::NetworkRunnerBase, GamePlay::Network::CustomNetworkRunner);
#pragma endregion

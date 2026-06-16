#pragma once
#define WIN32_LEAN_AND_MEAN
#include "../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../Core/Game/PlayerAvatar/SwordMan/CameraGroup/SwordManAvatarCameraGroup.h"
#include "../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../../Core/Network/Packet/Dispatcher/CustomPacketDispatcherGroup.h"

namespace GamePlay::Network
{
    class CustomNetworkRunner final : public Module::Network::NetworkRunnerBase
    {
    public:
        [[nodiscard]] GameCore::Network::CustomDispatcherGroup& CustomDispatcher();

        std::weak_ptr<GameCore::IPlayerAvatar> SpawnPlayerAvatar(
            GameCore::PlayerAvatar::PlayerAvatarType type,
            glm::vec3 position,
            glm::quat rotation);

    private:
        void DoInitialize() override;
        void DoDispatchReceivedPacket(const Core::Network::Packet& packet) override;
        [[nodiscard]] std::unique_ptr<Core::Network::INetworkSystem> DoCreateUseNetworkSystem() const override;
        
        std::optional<GameCore::Network::CustomDispatcherGroup> customDispatcherGroup_;
        [[serialize(1)]] FIELD(Asset::PlayerAvatarFactory) playerAvatarFactory_;
        [[serialize(2)]] FIELD(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup) swordmanCameraGroup_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
            void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<NetworkRunnerBase>(this));
            archive(CEREAL_NVP(playerAvatarFactory_));
            archive(CEREAL_NVP(swordmanCameraGroup_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<NetworkRunnerBase>(this));
            if (version >= 1) archive(CEREAL_NVP(playerAvatarFactory_));
            if (version >= 2) archive(CEREAL_NVP(swordmanCameraGroup_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Network::CustomNetworkRunner, 2);
CEREAL_REGISTER_TYPE(GamePlay::Network::CustomNetworkRunner);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Network::NetworkRunnerBase, GamePlay::Network::CustomNetworkRunner);
#pragma endregion

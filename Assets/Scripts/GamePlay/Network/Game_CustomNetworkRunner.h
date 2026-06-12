#pragma once
#include "../../../Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../Core/Network/Packet/Dispatcher/CustomPacketDispatcherGroup.h"

namespace GamePlay::Network
{
    class CustomNetworkRunner final : public Module::Network::NetworkRunnerBase
    {
    public:
        [[nodiscard]] GameCore::Network::CustomDispatcherGroup& CustomDispatcher();

    private:
        void DoInitialize() override;
        void DoDispatchReceivedPacket(const Core::Network::Packet& packet) override;
        [[nodiscard]] std::unique_ptr<Core::Network::INetworkSystem> DoCreateUseNetworkSystem() const override;
        
        std::optional<GameCore::Network::CustomDispatcherGroup> customDispatcherGroup_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
            void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<NetworkRunnerBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<NetworkRunnerBase>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Network::CustomNetworkRunner, 0);
CEREAL_REGISTER_TYPE(GamePlay::Network::CustomNetworkRunner);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Network::NetworkRunnerBase, GamePlay::Network::CustomNetworkRunner);
#pragma endregion

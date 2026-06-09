#pragma once
#include <memory>

#include "../../Core/Network/Engine_Network_INetworkSystem.h"
#include "../../Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../Component/ComponentBase.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace NanamiEngine::Module::Network
{
    /** Game実装側から呼ばれるAPIが実装されています。 */
    class NetworkRunnerBase : public Component::ComponentBase,
                              public LifeCycleCallback::IUpdatable
    {
    public:
        virtual ~NetworkRunnerBase() = default;
        
        /** API: 手動呼び出しの初期化 */
        void Initialize();
        /** API: Defaultで設定されているPacket割り当て処理一覧 */
        const Core::Network::DefaultPacketDispatcher& DefaultDispatcher();

    private:
        void OnUpdate() override;
        /** 受け取ったパケットを処理 */
        void DispatchPollPackets();

    protected:
        /** template method pattern*/
        virtual void DoInitialize() = 0;
        virtual void DoDispatchReceivedPacket(const Core::Network::Packet& packet) = 0;
        [[nodiscard]] virtual std::unique_ptr<Core::Network::INetworkSystem> DoCreateUseNetworkSystem() const = 0;
        
        /**SandBox pattern*/
        [[nodiscard]] Core::Network::IPacketSender    & PacketSender() const;
        [[nodiscard]] Core::Network::IPlayerIdProvider& PlayerIdProvider() const;
        
    private:
        std::optional<Core::Network::DefaultPacketDispatcher> defaultPacketDispatcher_;
        std::unique_ptr<Core::Network::INetworkSystem> networkSystem_;
        
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
CEREAL_CLASS_VERSION(Network::NetworkRunnerBase, 0);
CEREAL_REGISTER_TYPE(Network::NetworkRunnerBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, Network::NetworkRunnerBase);
#pragma endregion
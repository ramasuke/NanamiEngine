#pragma once
#include <memory>

#include "../../Core/Network/Engine_Network_INetworkSystem.h"
#include "../../Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#define WIN32_LEAN_AND_MEAN
#include "../../Core/Coroutine/Task/Task.h"
#include "../../Core/Object/Field/Field.h"
#include "../Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../Component/ComponentBase.h"

namespace NanamiEngine::Module::Network
{
    /** Game実装側から呼ばれるAPIが実装されています。 */
    class NetworkRunnerBase : public Component::ComponentBase,
                              public LifeCycleCallback::IUpdatable
    {
    public:
        NetworkRunnerBase();
        ~NetworkRunnerBase() override;

        [[nodiscard]] static NetworkRunnerBase& Instance();

        /** API: 手動呼び出しの初期化 */
        void Initialize();
        /** API: PlayerIDの取得 */
        [[nodiscard]] Core::Network::PlayerId GetPlayerId() const;
        /** API: Defaultで設定されているPacket割り当て処理一覧 */
        Core::Network::DefaultPacketDispatcher& DefaultDispatcher();
        /** API: パケット送信（NetworkTransform等のコンポーネントから呼ぶ） */
        void SendNetworkPacket(const Core::Network::Packet& packet);
        
        /** --- Defaultの通信処理API一覧 --- */
        //API: Network上のサーバーと接続してネットワーク上でクライアント登録されるまで待つAsync
        Coroutine::Task<void> OnConnectedAsync();
        //API: Network上で共有するオブジェクトの生成処理
        void Spawn(Asset::PrefabGameObjectFile& prefabFile, glm::vec3 position, glm::quat rotation);

    private:
        void OnUpdate() override;
        /** 受け取ったパケットを処理 */
        void DispatchPollPackets();

    protected:
        /** template method pattern*/
        virtual void DoInitialize() = 0;
        virtual void DoDispatchReceivedPacket(const Core::Network::Packet& packet) = 0;
        [[nodiscard]] virtual std::unique_ptr<Core::Network::INetworkSystem> DoCreateUseNetworkSystem() const = 0;
        
        /** SandBox pattern */
        [[nodiscard]] Core::Network::IPacketSender    & PacketSender() const;
        [[nodiscard]] Core::Network::IPlayerIdProvider& PlayerIdProvider() const;
        
    private:
        std::unique_ptr<Core::Network::INetworkSystem> networkSystem_;
        std::optional<Core::Network::DefaultPacketDispatcher> defaultPacketDispatcher_;
        [[serialize(1)]] FIELD(Asset::PrefabGameObjectFile) sampleSpawnPrefab_;

        static NetworkRunnerBase* s_instance_;
        
        
#pragma region Serialization Function
    public:
        void BasedOnDrawgui() override;

        template<class Archive>
            void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(sampleSpawnPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(sampleSpawnPrefab_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(Network::NetworkRunnerBase, 1)
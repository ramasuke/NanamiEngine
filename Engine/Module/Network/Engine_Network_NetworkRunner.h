#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

#include "../../Core/Network/Engine_Network_INetworkSystem.h"
#include "../../Core/Network/Mode/NetworkSystem_NetworkStartSettings.h"
#include "../../Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#define WIN32_LEAN_AND_MEAN
#include "../../Core/Coroutine/Task/Task.h"
#include "../../Core/Object/Field/Field.h"
#include "../Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../Component/ComponentBase.h"

namespace NanamiEngine::Core::Network
{
    class LanSessionAdvertiser;
}

namespace NanamiEngine::Module::Network
{
    // NOTE: 通信の開始・終了と送受信の入口。実際のシステムと振り分けは派生クラスが決める
    class NANAMI_API NetworkRunnerBase : public Component::ComponentBase,
                              public LifeCycleCallback::IUpdatable
    {
    public:
        NetworkRunnerBase();
        ~NetworkRunnerBase() override;

        [[nodiscard]] static NetworkRunnerBase& Instance();
        [[nodiscard]] static NetworkRunnerBase* TryGetInstance() { return s_instance_; }

        // NOTE: ホストとして開始し、sessionKey で LAN に告知する。結果は接続状態で見る
        void StartHost(const std::string& sessionKey);
        // NOTE: host へクライアントとして接続を始める。結果は接続状態で見る
        void StartClient(const Core::Network::HostEndpoint& host);
        // NOTE: 通信を止め、もう一度開始できる状態に戻す
        void Shutdown();
        [[nodiscard]] bool IsStarted() const;
        [[nodiscard]] bool IsServer() const;
        [[nodiscard]] Core::Network::ConnectionState GetConnectionState() const;
        [[nodiscard]] Core::Network::PlayerId GetPlayerId() const;
        [[nodiscard]] Core::Network::PlayerId OwnerOf(Core::Network::NetworkObjectId id) const;
        // NOTE: 自分がそのオブジェクトの所有者 (権威) か
        [[nodiscard]] bool IsLocallyOwned(Core::Network::NetworkObjectId id) const;
        Core::Network::DefaultPacketDispatcher& DefaultDispatcher();
        void SendNetworkPacket(const Core::Network::Packet& packet);
        
        // NOTE: ネットワーク上で共有するオブジェクトを生成する
        void Spawn(Asset::PrefabGameObjectFile& prefabFile, glm::vec3 position, glm::quat rotation);

    protected:
        // NOTE: settings は DoCreateUseNetworkSystem にそのまま渡る。独自の INetworkSystem で始める派生クラス向け
        void Start(const Core::Network::NetworkStartSettings& settings);

    private:
        void OnUpdate() override;
        void DispatchPollPackets();

    protected:
        // NOTE: 派生クラスが埋めるテンプレートメソッド
        virtual void DoInitialize() = 0;
        virtual void DoShutdown() = 0;
        virtual void DoDispatchReceivedPacket(const Core::Network::Packet& packet) = 0;
        [[nodiscard]] virtual std::unique_ptr<Core::Network::INetworkSystem> DoCreateUseNetworkSystem(
            const Core::Network::NetworkStartSettings& settings) const = 0;

        // NOTE: 派生クラスに公開する通信部品
        [[nodiscard]] Core::Network::IPacketSender    & PacketSender() const;
        [[nodiscard]] Core::Network::IPlayerIdProvider& PlayerIdProvider() const;

    private:
        std::unique_ptr<Core::Network::INetworkSystem> networkSystem_;
        std::optional<Core::Network::DefaultPacketDispatcher> defaultPacketDispatcher_;
        std::unique_ptr<Core::Network::LanSessionAdvertiser> lanAdvertiser_;
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

CEREAL_CLASS_VERSION(Network::NetworkRunnerBase, 1);

#pragma once
#define WIN32_LEAN_AND_MEAN
#include "../../../Data/Enemy/Factory/EnemyFactory.h"
#include "../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../Core/Game/PlayerAvatar/SwordMan/CameraGroup/SwordManAvatarCameraGroup.h"
#include "../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../../Core/Network/Packet/Dispatcher/CustomPacketDispatcherGroup.h"
#include "Relay/RelayRoom.h"
#include "Relay/RelayServerSettings.h"

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GamePlay::Network
{
    class CustomNetworkRunner final : public Module::Network::NetworkRunnerBase
    {
    public:
        [[nodiscard]] static CustomNetworkRunner& Instance()
        {
            return static_cast<CustomNetworkRunner&>(NetworkRunnerBase::Instance());
        }

        [[nodiscard]] GameCore::Network::CustomDispatcherGroup& CustomDispatcher();

        /**
         * 中継サーバー経由で sessionKey の部屋に入る。公開部屋なら空きが無ければホストになる。結果は GetConnectionState() で見る
         * @note 非公開部屋のコードと断られた理由は RelayRoomCode() / RelayFailure() で読める
         */
        void StartRelay(const std::string& sessionKey, const RelayServerSettings& relay, const RelayRoom& room = {});

        /** 非公開部屋のコード。公開部屋・LAN・未接続では空 */
        [[nodiscard]] std::string RelayRoomCode() const;
        /** 中継サーバーの部屋に入る前に切れた理由 */
        [[nodiscard]] std::optional<std::string> RelayFailure() const;

        std::weak_ptr<GameCore::IPlayerAvatar> SpawnPlayerAvatar(
            GameCore::PlayerAvatar::PlayerAvatarType type,
            glm::vec3 position,
            glm::quat rotation);

        std::shared_ptr<Module::GameObject::IGameObject> SpawnEnemy(
            GameCore::Npc::Enemy::EnemyKind kind,
            glm::vec3 position,
            glm::quat rotation);

    private:
        void DoInitialize() override;
        void DoShutdown() override;
        void DoDispatchReceivedPacket(const Core::Network::Packet& packet) override;
        [[nodiscard]] std::unique_ptr<Core::Network::INetworkSystem> DoCreateUseNetworkSystem(
            const Core::Network::NetworkStartSettings& settings) const override;
        
        struct RelayStart
        {
            std::string         sessionKey;
            RelayServerSettings settings;
            RelayRoom           room;
            std::shared_ptr<RelayRoomStatus> status;
        };

        std::optional<GameCore::Network::CustomDispatcherGroup> customDispatcherGroup_;
        // StartRelay の間だけ入り、DoCreateUseNetworkSystem が EnetRelayNetworkSystem を選ぶ目印になる
        std::optional<RelayStart> pendingRelayStart_;
        // インスペクタ表示用: 最後に中継サーバー経由で始めたときの接続先
        std::optional<RelayStart> activeRelay_;
        [[serialize(1)]] FIELD(Asset::PlayerAvatarFactory) playerAvatarFactory_;
        [[serialize(4)]] FIELD(Asset::EnemyFactory) enemyFactory_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
            void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<NetworkRunnerBase>(this));
            archive(CEREAL_NVP(playerAvatarFactory_));
            [[serialize(2)]] FIELD(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup) swordmanCameraGroup_;
            if (version == 2) archive(CEREAL_NVP(swordmanCameraGroup_));
            archive(CEREAL_NVP(enemyFactory_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<NetworkRunnerBase>(this));
            if (version >= 1) archive(CEREAL_NVP(playerAvatarFactory_));
            [[serialize(2)]] FIELD(GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup) swordmanCameraGroup_;
            if (version == 2) archive(CEREAL_NVP(swordmanCameraGroup_));
            if (version >= 4) archive(CEREAL_NVP(enemyFactory_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Network::CustomNetworkRunner, 4);
#pragma endregion

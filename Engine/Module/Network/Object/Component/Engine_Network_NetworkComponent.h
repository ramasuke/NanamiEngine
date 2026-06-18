#pragma once
#include "../../../../Core/Network/Object/INetworkObject.h"
#include "../../../../Core/Network/Object/Tickable/INetworkTickable.h"
#include "../../../../Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::Network
{
    class NetworkRunnerBase;

    class NetworkComponent : public Component::ComponentBase,
                             public Core::Network::INetworkObject,
                             public Core::Network::INetworkTickable
    {
    protected:
        /** API: 自身がSpawnしたオブジェクトかを判別する */
        [[nodiscard]] bool HasStateAuthority() const;
        /** API: このオブジェクトのNetworkObjectIdを取得する */
        [[nodiscard]] Core::Network::NetworkObjectId GetNetworkObjectId() const;
        /** API: シングルトンのNetworkRunnerへのサンドボックスアクセス */
        [[nodiscard]] NetworkRunnerBase& NetworkRunner() const;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Network::NetworkComponent, 0);
CEREAL_REGISTER_TYPE(Network::NetworkComponent);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, Network::NetworkComponent);
#pragma endregion

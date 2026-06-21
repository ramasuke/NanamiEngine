#pragma once
#include <memory>
#include <type_traits>
#include <vector>

#include "../../../../Core/Network/Object/Awakable/INetworkAwakable.h"
#include "../../../../Core/Network/Object/Tickable/INetworkTickable.h"
#include "../../../../Core/Network/Object/SyncParameter/Network_SyncParameter.h"
#include "../../../../Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::Network
{
    class NetworkRunnerBase;

    class NetworkComponent : public Component::ComponentBase,
                             public Core::Network::INetworkAwakable,
                             public Core::Network::INetworkTickable
    {
    public:
        void NetworkAwake(Core::Network::NetworkObjectId id) override;

    protected:
        template<typename T>
        using SyncParam = std::shared_ptr<Core::Network::SyncParameter<T>>;
        
        template <typename T>
        SyncParam<T> CreateSyncParameter(T defaultValue = T())
        {
            auto ptr = std::make_shared<Core::Network::SyncParameter<T>>(std::move(defaultValue));
            networkObjects_.push_back(ptr);
            return ptr;
        }

        template <typename T, typename... Args>
        std::shared_ptr<T> CreateSyncObject(Args&&... args)
        {
            static_assert(std::is_base_of_v<Core::Network::NetworkObjectBase, T>,
                          "T must inherit from NetworkObjectBase");
            auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
            networkObjects_.push_back(ptr);
            return ptr;
        }

        template <typename T>
        void RegisterSyncParameter(const std::shared_ptr<Core::Network::SyncParameter<T>>& param)
        {
            networkObjects_.push_back(param);
        }

        template <typename T>
        void RegisterSyncObject(const std::shared_ptr<T>& obj)
        {
            static_assert(std::is_base_of_v<Core::Network::NetworkObjectBase, T>,
                          "T must inherit from NetworkObjectBase");
            networkObjects_.push_back(obj);
        }

        /** API: 自身がSpawnしたオブジェクトかを判別する */
        [[nodiscard]] bool HasStateAuthority() const;
        /** API: このオブジェクトのNetworkObjectIdを取得する */
        [[nodiscard]] Core::Network::NetworkObjectId GetNetworkObjectId() const;
        /** API: シングルトンのNetworkRunnerへのサンドボックスアクセス */
        [[nodiscard]] NetworkRunnerBase& NetworkRunner() const;

    private:
        Core::Network::NetworkObjectId objectId_;
        uint32_t localIndex_ = 0;
        std::vector<std::shared_ptr<Core::Network::NetworkObjectBase>> networkObjects_;
        

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
ENGINE_REGISTER_COMPONENT(Network::NetworkComponent, 0)

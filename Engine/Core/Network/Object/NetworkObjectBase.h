#pragma once
#include <memory>
#include <type_traits>
#include <vector>

#include "INetworkObject.h"
#include "../ObjectId/Engine_Network_NetworkObjectId.h"

using namespace NanamiEngine::Core::Network;

namespace NanamiEngine::Core::Network
{
    /** ネットワーク上で共通の値を持つ可能性のあるオブジェクトに継承させるclass */
    class NetworkObjectBase : public INetworkObject
    {
    public:
        friend class SyncParamFactory;
        ~NetworkObjectBase() override;
        virtual void NetworkAwake(NetworkObjectId id, uint32_t& localIndex);

    protected:
        template <typename T, typename... Args>
        std::shared_ptr<T> CreateSyncObject(Args&&... args);
        template <typename T>
        void RegisterSyncObject(const std::shared_ptr<T>& obj);

    private:
        std::vector<std::shared_ptr<NetworkObjectBase>> networkObjects_;
    };
}

namespace NanamiEngine::Core::Network
{
    template <typename T, typename... Args>
    std::shared_ptr<T> NetworkObjectBase::CreateSyncObject(Args&&... args)
    {
        static_assert(std::is_base_of_v<NetworkObjectBase, T>, "T must inherit from NetworkObjectBase");
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        networkObjects_.push_back(ptr);
        return ptr;
    }

    template <typename T>
    void NetworkObjectBase::RegisterSyncObject(const std::shared_ptr<T>& obj)
    {
        static_assert(std::is_base_of_v<NetworkObjectBase, T>, "T must inherit from NetworkObjectBase");
        networkObjects_.push_back(obj);
    }
}

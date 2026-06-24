#pragma once
#include "../../../../../Core/Network/Object/INetworkObject.h"
#include "../../../../../Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::Network
{
    /** Network上で共通のPrefabとして扱うために使用するクラス */
    class NetworkGameObject final : public Component::ComponentBase,
                                    public Core::Network::INetworkObject
    {
    public:
        [[nodiscard]] Core::Network::NetworkObjectId GetNetworkObjectId() const { return networkObjectId_; }
        void SetNetworkObjectId(Core::Network::NetworkObjectId id);

    private:
        void InitNetworkObject() const;
        
        Core::Network::NetworkObjectId networkObjectId_ = Core::Network::NetworkObjectId::Invalid();

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}
ENGINE_REGISTER_COMPONENT(Network::NetworkGameObject, 0)

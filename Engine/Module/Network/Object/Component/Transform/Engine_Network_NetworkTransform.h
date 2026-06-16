#pragma once
#include "../../../../../Core/Network/Object/INetworkObject.h"
#include "../../../../../Core/Network/Object/Tickable/INetworkTickable.h"
#include "../../../../../Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../../Component/ComponentBase.h"
#include "../../../../LifeCycleCallback/Awake/IAwakable.h"

namespace NanamiEngine::Module::Network
{
    class NetworkTransform final : public Component::ComponentBase,
                                   public Core::Network::INetworkObject,
                                   public Core::Network::INetworkTickable
    {
    public:
        void NetworkedTick() override;

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
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Network::NetworkTransform, 0);
CEREAL_REGISTER_TYPE(Network::NetworkTransform);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, Network::NetworkTransform);
#pragma endregion
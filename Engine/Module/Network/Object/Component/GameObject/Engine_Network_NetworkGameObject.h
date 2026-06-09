#pragma once
#include "../../../../../Core/Network/Object/INetworkObject.h"
#include "../../../../Component/ComponentBase.h"

namespace NanamiEngine::Module::Network
{
    class NetworkGameObject final : public Component::ComponentBase,
                                    public Core::Network::INetworkObject
    {
    private:
        
        
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
CEREAL_CLASS_VERSION(Network::NetworkGameObject, 0);
CEREAL_REGISTER_TYPE(Network::NetworkGameObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, Network::NetworkGameObject);
#pragma endregion
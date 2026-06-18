#pragma once
#include "../Engine_Network_NetworkComponent.h"
#include "../../../../../Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Module::Network
{
    class NetworkTransform final : public NetworkComponent
    {
    public:
        void NetworkedTick() override;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<NetworkComponent>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version == 0)
                archive(cereal::base_class<Component::ComponentBase>(this));
            else
                archive(cereal::base_class<NetworkComponent>(this));
        }
#pragma endregion
    };
}
ENGINE_REGISTER_COMPONENT(Network::NetworkTransform, 1)

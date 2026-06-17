#pragma once
#include "../../../../../../Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"

namespace NanamiEngine::Module::Network
{
    class NetworkAnimator final : public NetworkComponent
    {
    public:
        void NetworkedTick() override;

#pragma region Serialization Function
    public:
        void OnDrawGui() override {}

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<NanamiEngine::Module::Network::NetworkComponent>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<NanamiEngine::Module::Network::NetworkComponent>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Network::NetworkAnimator, 0);
CEREAL_REGISTER_TYPE(Network::NetworkAnimator);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Network::NetworkComponent, Network::NetworkAnimator);
#pragma endregion

#pragma once
#include "../../../../../../Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"

namespace GamePlay::Npc::Enemy
{
    class NetworkBehaviourTree final : public NanamiEngine::Module::Network::NetworkComponent,
                                       public LifeCycleCallback::IAwakable
    {
    private:
        void OnAwake() override;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<NetworkComponent>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<NetworkComponent>(this));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::NetworkBehaviourTree, 1);
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::NetworkBehaviourTree);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Network::NetworkComponent, GamePlay::Npc::Enemy::NetworkBehaviourTree);
#pragma endregion

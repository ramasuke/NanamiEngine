#pragma once
#include "../../../../../../Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"
#include "../../../../../../Engine/Core/Network/Packet/ByteBuffer/Packet_ByteBuffer.h"
#include "../../../../../../Libs/rxcpp/operators/rx-all.hpp"

namespace GameCore::Npc::Enemy
{
    class BehaviourTree;
}

namespace GamePlay::Npc::Enemy
{
    class NetworkBehaviourTree final : public NanamiEngine::Module::Network::NetworkComponent,
                                       public LifeCycleCallback::IAwakable,
                                       public LifeCycleCallback::IStartable
    {
    public:
        void ApplyReceivedBuffer(
            const NanamiEngine::Core::Network::ByteBuffer& buffer, size_t& offset);

    private:
        void OnAwake() override;
        void OnStart() override;

    private:
        std::shared_ptr<GameCore::Npc::Enemy::BehaviourTree> syncBehaviour_;
        rxcpp::composite_subscription paramSubscription_;
        
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

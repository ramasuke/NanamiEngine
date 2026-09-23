#include "GamePlay_NetworkBehaviourTree.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Npc::Enemy
{
    void NetworkBehaviourTree::OnAwake()
    {
    }

    void NetworkBehaviourTree::OnDrawGui()
    {

    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::NetworkBehaviourTree);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Network::NetworkComponent, GamePlay::Npc::Enemy::NetworkBehaviourTree);
#pragma endregion

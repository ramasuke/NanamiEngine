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
NANAMI_REGISTER_TYPE(GamePlay::Npc::Enemy::NetworkBehaviourTree, Network::NetworkComponent);
#pragma endregion

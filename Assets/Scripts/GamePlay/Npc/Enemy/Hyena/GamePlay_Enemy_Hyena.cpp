#include "GamePlay_Enemy_Hyena.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

void GamePlay::Npc::Enemy::Hyena::DoAwake()
{
}


void GamePlay::Npc::Enemy::Hyena::DoUpdate()
{
    
}

void GamePlay::Npc::Enemy::Hyena::OnDrawGui()
{
    
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::Hyena);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::EnemyBase, GamePlay::Npc::Enemy::Hyena);
#pragma endregion

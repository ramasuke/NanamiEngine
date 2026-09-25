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
NANAMI_REGISTER_TYPE(GamePlay::Npc::Enemy::Hyena, GameCore::Npc::EnemyBase);
#pragma endregion

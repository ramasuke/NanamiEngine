#include "SampleEnemy.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy
{
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::SampleEnemy, GameCore::Npc::EnemyBase);
#pragma endregion

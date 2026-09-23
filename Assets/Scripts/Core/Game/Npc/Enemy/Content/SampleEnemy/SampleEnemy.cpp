#include "SampleEnemy.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy
{
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::SampleEnemy);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::EnemyBase, GameCore::Npc::Enemy::SampleEnemy);
#pragma endregion

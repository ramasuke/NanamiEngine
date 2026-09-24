#include "GamePlay_Enemy_SkeletonDragon.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::SkeletonDragon);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::BossEnemyBase, GamePlay::Npc::Enemy::SkeletonDragon);
#pragma endregion

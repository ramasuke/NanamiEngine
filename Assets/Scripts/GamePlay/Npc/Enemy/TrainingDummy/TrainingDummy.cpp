#include "TrainingDummy.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::TrainingDummy);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::EnemyBase, GamePlay::Npc::Enemy::TrainingDummy);
#pragma endregion

#include "EnemySpawnPoint.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy
{
    void EnemySpawnPoint::OnDrawGui()
    {
        ImGuiHelper::OnDrawEnumField("kind_", kind_, ENEMY_KINDS, ToString);
        ImGuiHelper::OnDrawInputField("prefab_", prefab_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::Npc::Enemy::EnemySpawnPoint);
#pragma endregion

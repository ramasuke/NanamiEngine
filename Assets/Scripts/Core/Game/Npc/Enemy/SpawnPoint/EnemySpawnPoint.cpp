#include "EnemySpawnPoint.h"

namespace GameCore::Npc::Enemy
{
    void EnemySpawnPoint::OnDrawGui()
    {
        ImGuiHelper::OnDrawEnumField("kind_", kind_, ENEMY_KINDS, ToString);
    }
}

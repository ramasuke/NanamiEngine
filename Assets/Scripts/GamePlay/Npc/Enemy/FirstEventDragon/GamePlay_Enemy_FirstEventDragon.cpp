#include "GamePlay_Enemy_FirstEventDragon.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Npc::Enemy
{
    void FirstEventDragon::DoUpdate()
    {
        if (Transform().GetWorldPos().y < -100)
        {
            Transform().SetLocalPos(glm::vec3{0.0f, 300.0f, 0.0f});
        }

    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::FirstEventDragon);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::BossEnemyBase, GamePlay::Npc::Enemy::FirstEventDragon);
#pragma endregion

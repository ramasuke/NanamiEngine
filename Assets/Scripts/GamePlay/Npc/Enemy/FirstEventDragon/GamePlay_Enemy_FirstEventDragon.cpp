#include "GamePlay_Enemy_FirstEventDragon.h"

#include "Engine/Module/GameObject/Transform/Transform.h"

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
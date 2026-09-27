#include "GamePlay_Enemy_AncientDragon.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Npc::Enemy
{
    void AncientDragon::DoUpdate()
    {
        if (Transform().GetWorldPos().y < fallLimitY_)
            Transform().SetWorldPos(respawnPosition_);
    }

    void AncientDragon::BasedOnDrawgui()
    {
        BossEnemyBase::BasedOnDrawgui();
        ImGuiHelper::OnDrawInputField("respawnPosition_", respawnPosition_);
        ImGuiHelper::OnDrawInputField("fallLimitY_", fallLimitY_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GamePlay::Npc::Enemy::AncientDragon, GameCore::Npc::BossEnemyBase);
#pragma endregion

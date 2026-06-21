#include "SwordManAvatar.h"

#include "../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
#include "../../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "../../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    std::weak_ptr<PlayerAttackArea> SwordManAvatar::CatchNormalAttackArea() const
    {
        return GameObject::CatchChild<PlayerAttackArea>(Entity(), NORMAL_ATTACK_AREA_NAME);
    }
    
    std::weak_ptr<PlayerAttackArea> SwordManAvatar::CatchDashAttackArea() const
    {
        return GameObject::CatchChild<PlayerAttackArea>(Entity(), DASH_ATTACK_AREA_NAME);
    }

    PlayerAvatarType SwordManAvatar::Type() const
    {
        return PlayerAvatarType::SwordMan;
    }

    void SwordManAvatar::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("resources_", resources_);
    }
}

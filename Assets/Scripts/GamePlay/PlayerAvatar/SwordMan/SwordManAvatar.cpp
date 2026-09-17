#include "SwordManAvatar.h"

#include "../../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "../../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    std::weak_ptr<PlayerAttackArea> SwordManAvatar::CatchNormalAttackArea() const
    {
        return normalAttackArea_.get();
    }

    std::weak_ptr<PlayerAttackArea> SwordManAvatar::CatchDashAttackArea() const
    {
        return dashAttackArea_.get();
    }

    std::weak_ptr<LockOnDetectionArea> SwordManAvatar::CatchLockOnDetectionArea() const
    {
        return lockOnDetectionArea_.get();
    }

    std::weak_ptr<Component::ParticleSystem> SwordManAvatar::CatchSuccessAvoidRollingParticle() const
    {
        return successAvoidRollingParticle_.get();
    }

    PlayerAvatarType SwordManAvatar::Type() const
    {
        return PlayerAvatarType::SwordMan;
    }

    void SwordManAvatar::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("resources_", resources_);
        ImGuiHelper::OnDrawInputField("normalAttackArea_", normalAttackArea_);
        ImGuiHelper::OnDrawInputField("dashAttackArea_", dashAttackArea_);
        ImGuiHelper::OnDrawInputField("lockOnDetectionArea_", lockOnDetectionArea_);
        ImGuiHelper::OnDrawInputField("successAvoidRollingParticle_", successAvoidRollingParticle_);
    }
}

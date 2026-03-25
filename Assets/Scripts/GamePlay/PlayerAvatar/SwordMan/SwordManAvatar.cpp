#include "SwordManAvatar.h"

#include "../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    std::weak_ptr<PlayerAttackArea> SwordManAvatar::CatchNormalAttackArea() const
    {
        for (const auto& child : Transform().GetChildren())
        {
            if (child->Name() == NORMAL_ATTACK_AREA_NAME)
                return child->Components().Catch<PlayerAttackArea>(); 
        }
        throw std::exception("not found featStepPosition");
    }
    
    std::weak_ptr<PlayerAttackArea> SwordManAvatar::CatchDashAttackArea() const
    {
        for (const auto& child : Transform().GetChildren())
        {
            if (child->Name() == DASH_ATTACK_AREA_NAME)
                return child->Components().Catch<PlayerAttackArea>();
        }
        throw std::exception("not found featStepPosition");
    }

    std::weak_ptr<Component::ParticleSystem> SwordManAvatar::OnReinforceParticle() const
    {
        for (const auto& child : Transform().GetChildren())
        {
            if (child->Name() == ON_REINFORCE_PARTICLE_NAME)
                return child->Components().Catch<Component::ParticleSystem>();
        }
        throw std::exception("not found ON_REINFORCE_PARTICLE_NAME");
    }

    std::weak_ptr<Component::ParticleSystem> SwordManAvatar::ReinforcingParticle() const
    {
        for (const auto& child : Transform().GetChildren())
        {
            if (child->Name() == REINFORCING_PARTICLE_NAME)
                return child->Components().Catch<Component::ParticleSystem>();
        }
        throw std::exception("not found REINFORCING_PARTICLE_NAME");
    }

    std::weak_ptr<Asset::SoundFile> SwordManAvatar::NormalAttackSound() const
    {
        return normalAttackSound_.get();
    }

    std::weak_ptr<Asset::SoundFile> SwordManAvatar::AvoidRollingSound() const
    {
        return avoidRollingSound_.get();
    }

    void SwordManAvatar::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("normalAttackSound_", normalAttackSound_);
        ImGuiHelper::OnDrawInputField("avoidRollingSound_", avoidRollingSound_);
    }
}

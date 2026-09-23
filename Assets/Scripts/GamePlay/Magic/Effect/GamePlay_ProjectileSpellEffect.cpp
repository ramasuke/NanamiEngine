#include "GamePlay_ProjectileSpellEffect.h"

#include <algorithm>

#include "geometric.hpp"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Magic/IMagicCaster.h"
#include "../Component/GamePlay_MagicProjectile.h"
#include "../GamePlay_MagicAim.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Magic
{
    namespace
    {
        /** 狙った点を通り過ぎても少しは飛ばし、ロックオン対象の手前で消えないようにする */
        constexpr float PROJECTILE_SPELL_LIFETIME_MARGIN = 1.2f;
    }

    GameCore::Magic::MagicCastTarget ProjectileSpellEffect::Aim(const GameCore::Magic::IMagicCaster& caster) const
    {
        GameCore::Magic::MagicCastTarget target;
        target.origin    = caster.CastOrigin();
        target.targetPos = AimPoint(caster, range_);
        target.rotation  = LookRotation(target.origin, target.targetPos, caster.CastRotation());
        target.powerRate = caster.SpellPowerRate();
        return target;
    }

    void ProjectileSpellEffect::Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const
    {
        if (!projectilePrefab_ || speed_ <= 0.0f)
            return;

        const auto projectile = Scene::GameObject::Instantiate(*projectilePrefab_.get(), target.origin, target.rotation).lock();
        if (!projectile)
            return;

        const auto magicProjectile = projectile->Components().Catch<MagicProjectile>().lock();
        if (!magicProjectile)
            return;

        const glm::vec3 direction = glm::normalize(glm::vec3(target.rotation * glm::vec3(0.0f, 0.0f, -1.0f)));
        const float flightDistance = (std::max)(glm::distance(target.origin, target.targetPos), range_);
        magicProjectile->Launch(caster.CasterObject(),
                                ScaledPower(power_, target.powerRate),
                                direction * speed_,
                                flightDistance / speed_ * PROJECTILE_SPELL_LIFETIME_MARGIN);
    }

    void ProjectileSpellEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("projectilePrefab_", projectilePrefab_);
        ImGuiHelper::OnDrawInputField("power_", power_);
        ImGuiHelper::OnDrawInputField("speed_", speed_);
        ImGuiHelper::OnDrawInputField("range_", range_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Magic::ProjectileSpellEffect);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Magic::IMagicSpellEffect, GamePlay::Magic::ProjectileSpellEffect);
#pragma endregion

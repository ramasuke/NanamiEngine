#include "GamePlay_PlacementSpellEffect.h"

#include "geometric.hpp"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Magic/IMagicCaster.h"
#include "../Component/GamePlay_MagicBlast.h"
#include "../Component/GamePlay_MagicPlacement.h"
#include "../GamePlay_MagicAim.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Magic
{
    GameCore::Magic::MagicCastTarget PlacementSpellEffect::Aim(const GameCore::Magic::IMagicCaster& caster) const
    {
        const auto casterObject = caster.CasterObject();
        const glm::vec3 casterPos = casterObject ? casterObject->Transform().GetWorldPos() : caster.CastOrigin();

        glm::vec3 direction = AimPoint(caster, distance_) - casterPos;
        direction.y = 0.0f;
        const float length = glm::length(direction);
        direction = length > 0.0001f ? direction / length : CasterForward(caster);

        GameCore::Magic::MagicCastTarget target;
        target.origin    = caster.CastOrigin();
        target.targetPos = ProjectToGround(casterPos + direction * distance_);
        target.rotation  = LookRotation(glm::vec3(0.0f), direction, caster.CastRotation());
        target.powerRate = caster.SpellPowerRate();
        return target;
    }

    void PlacementSpellEffect::Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const
    {
        if (!placementPrefab_)
            return;

        const auto placed = Scene::GameObject::Instantiate(*placementPrefab_.get(), target.targetPos, target.rotation).lock();
        if (!placed)
            return;

        if (const auto placement = placed->Components().Catch<MagicPlacement>().lock())
            placement->Place(lifeTime_secs_);
        if (const auto blast = placed->Components().Catch<MagicBlast>().lock())
            blast->Arm(caster.CasterObject(), ScaledPower(power_, target.powerRate), 0.0f);
    }

    void PlacementSpellEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("placementPrefab_", placementPrefab_);
        ImGuiHelper::OnDrawInputField("power_", power_);
        ImGuiHelper::OnDrawInputField("lifeTime_secs_", lifeTime_secs_);
        ImGuiHelper::OnDrawInputField("distance_", distance_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GamePlay::Magic::PlacementSpellEffect);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Magic::IMagicSpellEffect, GamePlay::Magic::PlacementSpellEffect);
#pragma endregion

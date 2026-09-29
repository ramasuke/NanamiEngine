#include "GamePlay_AreaSpellEffect.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Magic/IMagicCaster.h"
#include "../Component/GamePlay_MagicBlast.h"
#include "../GamePlay_MagicAim.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Magic
{
    GameCore::Magic::MagicCastTarget AreaSpellEffect::Aim(const GameCore::Magic::IMagicCaster& caster) const
    {
        GameCore::Magic::MagicCastTarget target;
        target.origin    = caster.CastOrigin();
        if (centerOnCaster_)
        {
            const auto casterObject = caster.CasterObject();
            target.targetPos = ProjectToGround(casterObject ? casterObject->Transform().GetWorldPos() : caster.CastOrigin());
        }
        else
        {
            target.targetPos = ProjectToGround(AimPoint(caster, range_));
        }
        target.rotation  = caster.CastRotation();
        target.powerRate = caster.SpellPowerRate();
        return target;
    }

    void AreaSpellEffect::Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const
    {
        if (!blastPrefab_)
            return;

        const auto blastObject = Scene::GameObject::Instantiate(*blastPrefab_.get(), target.targetPos, target.rotation).lock();
        if (!blastObject)
            return;

        if (const auto blast = blastObject->Components().Catch<MagicBlast>().lock())
            blast->Arm(caster.CasterObject(), ScaledPower(power_, target.powerRate), delay_secs_);
    }

    void AreaSpellEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("blastPrefab_", blastPrefab_);
        ImGuiHelper::OnDrawInputField("power_", power_);
        ImGuiHelper::OnDrawInputField("delay_secs_", delay_secs_);
        ImGuiHelper::OnDrawInputField("range_", range_);
        ImGuiHelper::OnDrawInputField("centerOnCaster_", centerOnCaster_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GamePlay::Magic::AreaSpellEffect, GameCore::Magic::IMagicSpellEffect);
#pragma endregion

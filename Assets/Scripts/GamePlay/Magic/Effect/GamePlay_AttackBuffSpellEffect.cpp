#include "GamePlay_AttackBuffSpellEffect.h"

#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Core/Game/Magic/IMagicCaster.h"
#include "../../../Core/Game/PlayerAvatar/Item/Effect/IItemEffectTarget.h"
#include "../../Spawn/GamePlay_PrefabSpawner.h"
#include "../GamePlay_MagicSupport.h"

namespace GamePlay::Magic
{
    GameCore::Magic::MagicCastTarget AttackBuffSpellEffect::Aim(const GameCore::Magic::IMagicCaster& caster) const
    {
        const auto casterObject = caster.CasterObject();

        GameCore::Magic::MagicCastTarget target;
        target.origin    = caster.CastOrigin();
        target.targetPos = casterObject ? casterObject->Transform().GetWorldPos() : caster.CastOrigin();
        target.rotation  = caster.CastRotation();
        target.powerRate = 1.0f;
        return target;
    }

    void AttackBuffSpellEffect::Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const
    {
        if (effectPrefab_)
            Spawn::SpawnPrefab(*effectPrefab_.get(), target.targetPos, effectLifeTime_secs_);

        ForEachSupportTarget(target.targetPos, radius_, [this](GameCore::PlayerAvatar::Item::IItemEffectTarget& effectTarget)
        {
            effectTarget.ApplyAttackBuff(rate_, duration_secs_);
        });
    }

    void AttackBuffSpellEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rate_", rate_);
        ImGuiHelper::OnDrawInputField("duration_secs_", duration_secs_);
        ImGuiHelper::OnDrawInputField("radius_", radius_);
        ImGuiHelper::OnDrawInputField("effectPrefab_", effectPrefab_);
        ImGuiHelper::OnDrawInputField("effectLifeTime_secs_", effectLifeTime_secs_);
    }
}

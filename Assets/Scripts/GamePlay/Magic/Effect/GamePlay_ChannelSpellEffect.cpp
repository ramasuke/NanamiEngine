#include "GamePlay_ChannelSpellEffect.h"

#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Magic/IMagicCaster.h"
#include "../Component/GamePlay_MagicChannel.h"
#include "../GamePlay_MagicAim.h"

namespace GamePlay::Magic
{
    GameCore::Magic::MagicCastTarget ChannelSpellEffect::Aim(const GameCore::Magic::IMagicCaster& caster) const
    {
        GameCore::Magic::MagicCastTarget target;
        target.origin    = caster.CastOrigin();
        target.targetPos = AimPoint(caster, range_);
        target.rotation  = LookRotation(target.origin, target.targetPos, caster.CastRotation());
        target.powerRate = caster.SpellPowerRate();
        return target;
    }

    void ChannelSpellEffect::Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const
    {
        if (!channelPrefab_)
            return;

        const auto channelObject = Scene::GameObject::Instantiate(*channelPrefab_.get(), target.origin, target.rotation).lock();
        if (!channelObject)
            return;

        if (const auto channel = channelObject->Components().Catch<MagicChannel>().lock())
            channel->Begin(caster.CasterObject(), ScaledPower(power_, target.powerRate), duration_secs_, tickInterval_secs_);
    }

    void ChannelSpellEffect::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("channelPrefab_", channelPrefab_);
        ImGuiHelper::OnDrawInputField("power_", power_);
        ImGuiHelper::OnDrawInputField("duration_secs_", duration_secs_);
        ImGuiHelper::OnDrawInputField("tickInterval_secs_", tickInterval_secs_);
        ImGuiHelper::OnDrawInputField("range_", range_);
    }
}

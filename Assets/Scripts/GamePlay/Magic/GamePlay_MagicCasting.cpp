#include "GamePlay_MagicCasting.h"

#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "../../Core/Game/Magic/IMagicCaster.h"
#include "../../Core/Game/Magic/IMagicSpell.h"
#include "../../Core/Game/Magic/MagicCastTarget.h"
#include "../../Core/Network/Rpc/Custom_RpcType.h"
#include "../Sound/SoundPlayer.h"

namespace GamePlay::Magic
{
    void CastSpell(
        const GameCore::Magic::IMagicSpell& spell,
        const GameCore::Magic::IMagicCaster& caster)
    {
        const auto target = spell.Aim(caster);
        ExecuteSpell(spell, caster, target);

        const auto casterId = caster.CasterNetworkObjectId();
        if (casterId == Core::Network::NetworkObjectId::Invalid())
            return;
        
        if (!Module::Network::NetworkRunnerBase::TryGetInstance())
            return;

        GameCore::Network::CastSpellRpc::Send(
            casterId, Core::Network::DeliveryMode::Reliable,
            spell.SpellGuid(), target.origin, target.rotation, target.targetPos, target.powerRate);
    }

    void ExecuteSpell(const GameCore::Magic::IMagicSpell& spell,
                      const GameCore::Magic::IMagicCaster& caster,
                      const GameCore::Magic::MagicCastTarget& target)
    {
        if (const auto sound = spell.CastSound())
            Sound::SoundPlayer::PlaySe(*sound, target.origin);

        spell.Execute(caster, target);
    }
}

#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../../../Data/Magic/Data_MagicSpellData.h"
#include "../../../../../GamePlay/Magic/GamePlay_MagicCasting.h"
#include "../../../../Game/Magic/IMagicCaster.h"
#include "../../../../Game/Magic/MagicCastTarget.h"

namespace
{
    struct CastSpellRpcRegistration
    {
        CastSpellRpcRegistration()
        {
            GameCore::Network::CastSpellRpc::OnTargeted<GameCore::Magic::IMagicCaster>(
                [](GameCore::Magic::IMagicCaster& caster,
                   Guid spellGuid, glm::vec3 origin, glm::quat rotation, glm::vec3 targetPos, float powerRate)
                {
                    const auto spell = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::MagicSpellData>(spellGuid).lock();
                    if (!spell)
                    {
                        NanamiEngine::Module::LogWarning("CastSpellRpc: MagicSpellData が見つかりません (guid:" + spellGuid.Value() + ")");
                        return;
                    }

                    GameCore::Magic::MagicCastTarget target;
                    target.origin    = origin;
                    target.rotation  = rotation;
                    target.targetPos = targetPos;
                    target.powerRate = powerRate;
                    GamePlay::Magic::ExecuteSpell(*spell, caster, target);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static CastSpellRpcRegistration s_castSpellRpcRegistration;
}

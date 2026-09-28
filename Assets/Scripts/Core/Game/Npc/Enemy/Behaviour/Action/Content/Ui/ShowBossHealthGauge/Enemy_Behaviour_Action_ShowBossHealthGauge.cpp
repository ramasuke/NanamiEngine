#include "Enemy_Behaviour_Action_ShowBossHealthGauge.h"

#include "../../../../../ShowHealthGaugeProvider/IShowHealthGaugeProvider.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ShowBossHealthGauge::DoTick(const TickContext& context)
    {
        const auto provider = context.ShowHealthGaugeProvider();
        if (!provider)
            return TickStatus::Failure;

        provider->ShowBossHealthGauge();

        if (context.IsNetworkAuthority())
            GameCore::Network::ShowBossHealthGaugeRpc::Send(context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable);
        return TickStatus::Success;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ShowBossHealthGauge, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion

#include "Enemy_Behaviour_Action_ShowBossHealthGauge.h"

#include "../../../../../ShowHealthGaugeProvider/IShowHealthGaugeProvider.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ShowBossHealthGauge::DoTick(const TickContext& context)
    {
        const auto provider = context.ShowHealthGaugeProvider();
        if (!provider)
            return TickStatus::Failure;

        provider->ShowBossHealthGauge();
        return TickStatus::Success;
    }
}

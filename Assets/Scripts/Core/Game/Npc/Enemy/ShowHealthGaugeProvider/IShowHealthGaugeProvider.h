#pragma once

namespace GameCore::Npc::Enemy
{
    class IShowHealthGaugeProvider
    {
    public:
        virtual ~IShowHealthGaugeProvider() = default;
        virtual void ShowBossHealthGauge() = 0;
    };
}

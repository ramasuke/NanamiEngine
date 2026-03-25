#include "Enemy_Behaviour_Action_Random.h"

#include <random>
#include <algorithm>

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::Random::DoTick(const TickContext&)
    {
        const int rate = std::clamp(successRate_, 0, 100);
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_int_distribution dist(0, 99);

        const int randomValue = dist(rng);
        return randomValue < rate ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::Random::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("Success Rate (%)", successRate_);
    }
}

#include "Friendly_Behaviour_Action_SampleFirstEventDragonChat.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus SampleFirstEventDragonChat::DoTick(const TickContext& context)
    {
        AppearFirstEventDragon(context);
        return TickStatus::Success; 
    }

    void SampleFirstEventDragonChat::AppearFirstEventDragon(const TickContext& context)
    {
        if (!enemyFactory_)
            return;

        enemyFactory_->Summon(enemyKind_, appearFirstEventDragonPosition_, glm::quat());
    }

    void SampleFirstEventDragonChat::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("enemyFactory_", enemyFactory_);
        ImGuiHelper::OnDrawEnumField("enemyKind_", enemyKind_, Enemy::ENEMY_KINDS, Enemy::ToString);
        ImGuiHelper::OnDrawInputField("AppearFirstEventDragonPosition_", appearFirstEventDragonPosition_);
    }
}

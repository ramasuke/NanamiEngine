#include "Enemy_Behaviour_Action_ChangeIsGravity.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeIsGravity::DoTick(const TickContext& context)
    {
        context.EnemyCollider().SetGravity(isGravity_);
        return TickStatus::Success;
    }

    void Action::ChangeIsGravity::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("isGravity_", isGravity_);
    }
}

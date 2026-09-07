#include "Npc_BehaviourNodeBase.h"

#include "ImGuiHelper.h"

namespace Editor::Npc::Behaviour
{
    const auto NODE_SIZE = ImVec2(120, 60);

    void NodeBase::ResetGuid()
    {
        guid_ = Guid();
    }

    void NodeBase::OnDrawGui()
    {
        ImGui::Text(("guid_: " + guid_.Value()).c_str());
        DoOnDrawGui();
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus NodeBase::Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        lastEnemyTickStatus_ = DoTick(context);
        hasBeenTickedAsEnemy_ = true;
        return lastEnemyTickStatus_;
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus NodeBase::Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        lastFriendlyTickStatus_ = DoTick(context);
        hasBeenTickedAsFriendly_ = true;
        return lastFriendlyTickStatus_;
    }
}

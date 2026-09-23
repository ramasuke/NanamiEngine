#include "Npc_BehaviourNodeBase.h"

#include "ImGuiHelper.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace Editor::Npc::Behaviour
{
    const auto NODE_SIZE = ImVec2(120, 60);

    void NodeBase::ResetGuid()
    {
        guid_ = Guid();
    }

    void NodeBase::ResetRuntimeState()
    {
        DoResetRuntimeState();
        for (const auto& child : Children())
        {
            if (child)
                child->ResetRuntimeState();
        }
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

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(Editor::Npc::Behaviour::NodeBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, Editor::Npc::Behaviour::NodeBase);
#pragma endregion

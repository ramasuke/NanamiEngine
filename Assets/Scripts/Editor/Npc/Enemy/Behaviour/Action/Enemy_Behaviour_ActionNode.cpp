#include "Enemy_Behaviour_ActionNode.h"

#include "Enemy_Behaviour_ActionFactory.h"
#include <typeinfo>

#include "../../../../BehaviourTree/Window/Node/Npc_Behaviour_NodeHeaders.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "Enemy_Behaviour_ActionHeaders.h"
#include "Engine/Module/Gui/StaticReflection/Engine_Module_StaticReflection.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"
#include "cereal/archives/portable_binary.hpp"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace Editor::Npc::Enemy::Behaviour
{
    ActionNode::ActionNode(std::unique_ptr<GameCore::Npc::Enemy::Behaviour::ActionBase> action)
        : action_(std::move(action))
    {
        
    }

    std::string ActionNode::GraphNodeDetail() const
    {
        if (!action_)
            return "(no action)";

        // NOTE: typeid の名前は "class A::B::Name" なので、最後の型名だけを出す
        const std::string typeName = typeid(*action_).name();
        const auto separator = typeName.find_last_of(": ");
        return separator == std::string::npos ? typeName : typeName.substr(separator + 1);
    }

    void ActionNode::DrawGraphContextMenuItems()
    {
        if (!ImGui::BeginMenu("Action"))
            return;

        const auto& actions = ActionFactory::Instance().CreatableActions();
        auto tree = StaticReflection::BuildTree<GameCore::Npc::Enemy::Behaviour::ActionBase>(actions);
        DrawTreeGui(tree, action_);
        ImGui::EndMenu();
    }
    
    GameCore::Npc::Enemy::Behaviour::TickStatus ActionNode::DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        return action_->Tick(context);
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus ActionNode::DoTick(
        const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        return GameCore::Npc::Friendly::Behaviour::TickStatus::Failure;
    }

    void ActionNode::DoResetRuntimeState()
    {
        if (action_)
            action_->Reset();
    }

    void ActionNode::SetConnectToNextNode(const std::shared_ptr<NodeBase> nextNode)
    {
        
    }

    void ActionNode::DoOnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("name_", name_);
        if (action_)
            action_->OnDrawGui();
    }
    
    template <class Archive>
    void ActionNode::save(Archive& archive, const std::uint32_t version) const
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(name_));
        archive(CEREAL_NVP(action_));
    }

    template <class Archive>
    void ActionNode::load(Archive& archive, const std::uint32_t version)
    {
        archive(cereal::base_class<NodeBase>(this));
        if (version >= 0) archive(CEREAL_NVP(name_));
        if (version >= 0) archive(CEREAL_NVP(action_));
    }
    
    template void ActionNode::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void ActionNode::load<cereal::JSONInputArchive >(cereal::JSONInputArchive&, const std::uint32_t);
    template void ActionNode::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void ActionNode::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(Editor::Npc::Enemy::Behaviour::ActionNode, Editor::Npc::Behaviour::NodeBase);
#pragma endregion

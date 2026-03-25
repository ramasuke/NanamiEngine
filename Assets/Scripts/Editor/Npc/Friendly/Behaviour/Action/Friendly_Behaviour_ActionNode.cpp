#include "Friendly_Behaviour_ActionNode.h"

#include "Friendly_Behaviour_ActionFactory.h"
#include "imgui_internal.h"
#include "../../../../BehaviourTree/Window/DrawNodeHelper.h"
#include "../../../../../../../Engine/Module/Gui/Graph/GraphGui.h"
#include "../../../../BehaviourTree/Window/Node/Npc_Behaviour_NodeHeaders.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "Friendly_Behaviour_ActionHeaders.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionBase.h"
#include "cereal/archives/portable_binary.hpp"

namespace Editor::Npc::Friendly::Behaviour
{
    ActionNode::ActionNode(std::unique_ptr<GameCore::Npc::Friendly::Behaviour::ActionBase> action)
        : action_(std::move(action))
    {
        
    }

    void ActionNode::OnDrawGraphEditorGui(
        const ImVec2& offset,
        ImDrawList* drawList,
        const std::weak_ptr<NodeBase>& ownPtr)
    {
        const Gui::Graph::NodeOption nodeOption
        {
            NODE_VISUAL_STYLE,
            name_,
            false,
            false,
            Npc::Behaviour::NODE_SIZE
        };

        Npc::Behaviour::DrawGraphEditorGuiHelper::DrawNode(ownPtr, offset, PositionRef(), drawList, nodeOption, false);
        
        const ImVec2 nodeSize = nodeOption.Size();
        const auto position = ImVec2(PositionRef().x, PositionRef().y);
        const ImRect nodeRect(offset + position, offset + position + nodeSize);

        // マウスがノード上にあるか
        const bool hovered = ImGui::IsMouseHoveringRect(nodeRect.Min, nodeRect.Max);
        // 右クリックでポップアップを開く
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(("ActionContextMenu##" + GetGuid().Value()).c_str());
        }
        
        // --- ポップアップ ---
        if (ImGui::BeginPopup(("ActionContextMenu##" + GetGuid().Value()).c_str()))
        {
            ImGui::TextUnformatted("Action");
            ImGui::Separator();

            // Factory から全ノードを列挙
            const auto& creatableActions = ActionFactory::Instance().CreatableActions();
            for (const auto& [typeName, createFunc] : creatableActions)
            {
                if (ImGui::Button(typeName.c_str()))
                {
                     action_ = std::move(createFunc());
                }
            }

            ImGui::EndPopup();
        }
    }
    
    GameCore::Npc::Enemy::Behaviour::TickStatus ActionNode::Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        return GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus ActionNode::Tick(
        const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        return action_->Tick(context);
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
    template void ActionNode::load<cereal::JSONInputArchive >(cereal::JSONInputArchive &, const std::uint32_t);
    template void ActionNode::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void ActionNode::load<cereal::PortableBinaryInputArchive >(cereal::PortableBinaryInputArchive &, const std::uint32_t);
}

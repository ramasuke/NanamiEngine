#include "Npc_BehaviourEntryNode.h"

#include "../../DrawNodeHelper.h"
#include "../../../../../../../Engine/Module/Gui/Graph/GraphGui.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "../Npc_Behaviour_NodeHeaders.h"
#include "../cereal/include/cereal/archives/portable_binary.hpp"

namespace Editor::Npc::Behaviour
{
    EntryNode::EntryNode()
        : nextNode_(nullptr)
    {
        
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus EntryNode::Tick(
        const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        return nextNode_->Tick(context);
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus EntryNode::Tick(
        const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        return nextNode_->Tick(context);
    }

    void EntryNode::OnDrawGraphEditorGui(
        const ImVec2& offset,
        ImDrawList* drawList,
        const std::weak_ptr<NodeBase>& ownPtr)
    {
        const Gui::Graph::NodeOption nodeOption
        {
            NODE_VISUAL_STYLE,
            ENTRY_NODE_NAME,
            false,
            false,
            NODE_SIZE
        };

        DrawGraphEditorGuiHelper::DrawNode(ownPtr, offset, PositionRef(), drawList, nodeOption, true);
        
        if (nextNode_)
        {
            DrawGraphEditorGuiHelper::DrawNodePath(offset, *ownPtr.lock(), *nextNode_, drawList);
            nextNode_->OnDrawGraphEditorGui(offset, drawList, nextNode_);
        }
    }

    void EntryNode::SetConnectToNextNode(const std::shared_ptr<NodeBase> nextNode)
    {
        nextNode_ = nextNode;
    }

    void EntryNode::DoOnDrawGui()
    {
        if (!nextNode_)
            return;

        ImGui::Separator();

        if (ImGui::Button("Disconnect Next"))
        {
            nextNode_.reset();
        }
    }
    
    template <class Archive>
    void EntryNode::save(Archive& archive, const std::uint32_t version) const
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(nextNode_));
    }

    template <class Archive>
    void EntryNode::load(Archive& archive, const std::uint32_t version)
    {
        archive(cereal::base_class<NodeBase>(this));
        if (version >= 0) archive(CEREAL_NVP(nextNode_));   
    }
    
    template void EntryNode::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void EntryNode::load<cereal::JSONInputArchive >(cereal::JSONInputArchive&, const std::uint32_t);
    template void EntryNode::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void EntryNode::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}

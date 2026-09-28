#include "Npc_BehaviourEntryNode.h"

#include "../cereal/include/cereal/archives/json.hpp"
#include "../Npc_Behaviour_NodeHeaders.h"
#include "../cereal/include/cereal/archives/portable_binary.hpp"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace Editor::Npc::Behaviour
{
    EntryNode::EntryNode()
        : nextNode_(nullptr)
    {
        
    }

    const std::string& EntryNode::NodeName() const
    {
        static const std::string NAME = "EntryNode";
        return NAME;
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus EntryNode::DoTick(
        const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        // NOTE: グラフエディタで先頭の子を切り離したまま保存されたツリーでも落ちないようにする
        if (!nextNode_)
            return GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;
        return nextNode_->Tick(context);
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus EntryNode::DoTick(
        const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        if (!nextNode_)
            return GameCore::Npc::Friendly::Behaviour::TickStatus::Failure;
        return nextNode_->Tick(context);
    }

    void EntryNode::SetConnectToNextNode(const std::shared_ptr<NodeBase> nextNode)
    {
        nextNode_ = nextNode;
    }

    std::optional<ChildSlot> EntryNode::RemoveChild(const NodeBase* child)
    {
        if (!nextNode_ || nextNode_.get() != child)
            return std::nullopt;

        nextNode_.reset();
        return ChildSlot{};
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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(Editor::Npc::Behaviour::EntryNode, Editor::Npc::Behaviour::NodeBase);
#pragma endregion

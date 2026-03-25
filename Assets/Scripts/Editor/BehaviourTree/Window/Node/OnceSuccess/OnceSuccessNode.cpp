#include "OnceSuccessNode.h"

#include "../../DrawNodeHelper.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/NodeOption.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "../cereal/include/cereal/archives/portable_binary.hpp"

namespace Editor::Npc::Behaviour
{
    const std::string& OnceSuccessNode::NodeName() const
    {
        static const std::string NAME = "OnceSuccessNode";
        return NAME;
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus
    OnceSuccessNode::Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        if (!child_)
            return GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;

        const auto result = child_->Tick(context);

        if (result == GameCore::Npc::Enemy::Behaviour::TickStatus::Running)
            return GameCore::Npc::Enemy::Behaviour::TickStatus::Running;

        if (!hasSucceeded_ && result == GameCore::Npc::Enemy::Behaviour::TickStatus::Success)
        {
            hasSucceeded_ = true;
            return GameCore::Npc::Enemy::Behaviour::TickStatus::Success;
        }

        return GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus
    OnceSuccessNode::Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        if (!child_)
            return GameCore::Npc::Friendly::Behaviour::TickStatus::Failure;

        const auto result = child_->Tick(context);

        if (result == GameCore::Npc::Friendly::Behaviour::TickStatus::Running)
            return GameCore::Npc::Friendly::Behaviour::TickStatus::Running;

        if (!hasSucceeded_ && result == GameCore::Npc::Friendly::Behaviour::TickStatus::Success)
        {
            hasSucceeded_ = true;
            return GameCore::Npc::Friendly::Behaviour::TickStatus::Success;
        }

        return GameCore::Npc::Friendly::Behaviour::TickStatus::Failure;
    }

    void OnceSuccessNode::SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode)
    {
        child_ = std::move(nextNode);
    }

    void OnceSuccessNode::OnDrawGraphEditorGui(
        const ImVec2& offset, ImDrawList* drawList,
        const std::weak_ptr<NodeBase>& ownPtr)
    {
        const Gui::Graph::NodeOption nodeOption
                {
                    NODE_VISUAL_STYLE,
                    "OnceSuccessNode",
                    true,
                    false,
                    NODE_SIZE
                };

        DrawGraphEditorGuiHelper::DrawNode(ownPtr, offset, PositionRef(), drawList, nodeOption, true);

        if (child_)
        {
            DrawGraphEditorGuiHelper::DrawNodePath(offset, *ownPtr.lock(), *child_, drawList);
            child_->OnDrawGraphEditorGui(offset, drawList, child_);
        }
    }

    void OnceSuccessNode::DoOnDrawGui()
    {
        ImGui::Text("Once Success Decorator");
        ImGui::Separator();

        ImGui::Text("State : %s", hasSucceeded_ ? "Succeeded" : "Not Succeeded");

        if (!child_)
        {
            ImGui::TextDisabled("No Child Node");
        }
        else
        {
            ImGui::Text("Child : %s", child_->NodeName().c_str());
        }
    }
    
    template<class Archive>
    void OnceSuccessNode::save(Archive& archive, const std::uint32_t version) const
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(child_));
    }

    template<class Archive>
    void OnceSuccessNode::load(Archive& archive, const std::uint32_t version)
    {
        archive(cereal::base_class<NodeBase>(this));
        if (version >= 0) archive(CEREAL_NVP(child_));
    }

    template void OnceSuccessNode::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void OnceSuccessNode::load<cereal::JSONInputArchive >(cereal::JSONInputArchive&, const std::uint32_t);
    template void OnceSuccessNode::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void OnceSuccessNode::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}

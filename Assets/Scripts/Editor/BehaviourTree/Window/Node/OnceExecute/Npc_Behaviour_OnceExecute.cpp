#include "Npc_Behaviour_OnceExecute.h"

#include "../../DrawNodeHelper.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/NodeOption.h"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "../"

namespace Editor::Npc::Behaviour
{
    const std::string& OnceExecute::NodeName() const
    {
        static const std::string NAME = "OnceExecute";
        return NAME;
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus OnceExecute::Tick(
        const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        return TickImpl<
            GameCore::Npc::Enemy::Behaviour::Action::TickContext,
            GameCore::Npc::Enemy::Behaviour::TickStatus>(context);
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus OnceExecute::Tick(
        const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        return TickImpl<
            GameCore::Npc::Friendly::Behaviour::Action::TickContext,
            GameCore::Npc::Friendly::Behaviour::TickStatus>(context);
    }

    void OnceExecute::SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode)
    {
        child_ = std::move(nextNode);
    }

    void OnceExecute::OnDrawGraphEditorGui(
        const ImVec2& offset,
        ImDrawList* drawList,
        const std::weak_ptr<NodeBase>& ownPtr)
    {
        const Gui::Graph::NodeOption nodeOption
        {
            NODE_VISUAL_STYLE,
            "OnceExecute",
            true,
            false,
            NODE_SIZE
        };

        DrawGraphEditorGuiHelper::DrawNode(
            ownPtr,
            offset,
            PositionRef(),
            drawList,
            nodeOption,
            true
        );

        if (child_)
        {
            DrawGraphEditorGuiHelper::DrawNodePath(
                offset,
                *ownPtr.lock(),
                *child_,
                drawList
            );

            child_->OnDrawGraphEditorGui(offset, drawList, child_);
        }
    }

    void OnceExecute::DoOnDrawGui()
    {
        ImGui::Text("Once Execute Decorator");
        ImGui::Separator();
        auto stateStr = "Unknown";

        switch (state_)
        {
        case State::NotExecuted: stateStr = "Not Executed"; break;
        case State::Running:     stateStr = "Running"; break;
        case State::Success:     stateStr = "Success"; break;
        case State::Failure:     stateStr = "Failure"; break;
        }

        ImGui::Text("State : %s", stateStr);

        if (!child_)
        {
            ImGui::TextDisabled("No Child Node");
        }
        else
        {
            ImGui::Text("Child : %s", child_->NodeName().c_str());
        }
    }

    template <class Archive>
    void OnceExecute::save(Archive& archive, const std::uint32_t version) const
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(child_));
        archive(CEREAL_NVP(state_));
    }

    template <class Archive>
    void OnceExecute::load(Archive& archive, const std::uint32_t version)
    {
        archive(cereal::base_class<NodeBase>(this));

        if (version >= 0) archive(CEREAL_NVP(child_));
        if (version >= 0) archive(CEREAL_NVP(state_));
    }
    
    template void OnceExecute::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void OnceExecute::load<cereal::JSONInputArchive >(cereal::JSONInputArchive&, const std::uint32_t);
    template void OnceExecute::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void OnceExecute::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}

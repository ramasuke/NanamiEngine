#include "Npc_Behaviour_SelectorNode.h"

#include <format>

#include "../../DrawNodeHelper.h"
#include "../../../../../../../Engine/Module/Gui/Graph/GraphGui.h"
#include "../Npc_Behaviour_NodeHeaders.h"
#include <../cereal/include/cereal/types/vector.hpp>

#include "../../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "../cereal/include/cereal/archives/portable_binary.hpp"

namespace Editor::Npc::Behaviour
{
    void SelectorNode::OnDrawGraphEditorGui(
        const ImVec2& offset,
        ImDrawList* drawList,
        const std::weak_ptr<NodeBase>& ownPtr)
    {
        const Gui::Graph::NodeOption nodeOption
        {
            NODE_VISUAL_STYLE,
            "Selector",
            true,
            false,
            NODE_SIZE
        };

        DrawGraphEditorGuiHelper::DrawNode(ownPtr, offset, PositionRef(), drawList, nodeOption, true);

        for (const auto& child : children_)
        {
            DrawGraphEditorGuiHelper::DrawNodePath(
                offset, *ownPtr.lock(), *child, drawList);

            child->OnDrawGraphEditorGui(offset, drawList, child);
        }
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus SelectorNode::Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        using TickStatus = GameCore::Npc::Enemy::Behaviour::TickStatus;

        for (const auto& child : children_)
        {
            switch (child->Tick(context))
            {
            case TickStatus::Success:
                return TickStatus::Success;

            case TickStatus::Running:
                return TickStatus::Running;

            case TickStatus::Abort:
                return TickStatus::Abort;

            case TickStatus::Failure:
                break;
            }
        }

        return TickStatus::Failure;
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus SelectorNode::Tick(
        const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        using TickStatus = GameCore::Npc::Friendly::Behaviour::TickStatus;

        for (const auto& child : children_)
        {
            switch (child->Tick(context))
            {
            case TickStatus::Success:
                return TickStatus::Success;

            case TickStatus::Running:
                return TickStatus::Running;

            case TickStatus::Abort:
                return TickStatus::Abort;

            case TickStatus::Failure:
                break;
            }
        }

        return TickStatus::Failure;
    }

    void SelectorNode::SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode)
    {
        children_.push_back(std::move(nextNode));
    }

    void SelectorNode::DoOnDrawGui()
    {
        if (children_.empty())
        {
            ImGui::TextDisabled("No children");
            return;
        }

        ImGui::Text("Children");
        ImGui::Separator();

        for (size_t i = 0; i < children_.size();)
        {
            ImGui::PushID(static_cast<int>(i));

            // 並び替えボタン
            if (ImGui::ArrowButton("Up", ImGuiDir_Up))
            {
                if (i > 0) std::swap(children_[i], children_[i - 1]);
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("Down", ImGuiDir_Down))
            {
                if (i + 1 < children_.size())
                    std::swap(children_[i], children_[i + 1]);
            }
            ImGui::SameLine();

            //表示
            const std::string label = std::format("Child {} : {}", i, children_[i]->NodeName());

            ImGui::Selectable(label.c_str(), false);

            //右クリックメニュー
            if (ImGui::BeginPopupContextItem("ChildContext"))
            {
                if (ImGui::MenuItem("Delete"))
                {
                    children_.erase(children_.begin() + i);
                    ImGui::EndPopup();
                    ImGui::PopID();
                    continue;
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
            ++i;
        }
    }

    template<class Archive>
    void SelectorNode::save(Archive& archive, const std::uint32_t) const
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(children_));
    }

    template<class Archive>
    void SelectorNode::load(Archive& archive, const std::uint32_t)
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(children_));
    }
    template void SelectorNode::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void SelectorNode::load<cereal::JSONInputArchive >(cereal::JSONInputArchive&, const std::uint32_t);
    template void SelectorNode::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void SelectorNode::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}

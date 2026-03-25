#include "Npc_Behaviour_RandomSelector.h"

#include "../../DrawNodeHelper.h"
#include "../../../../../../../Engine/Module/Gui/Graph/GraphGui.h"
#include "../Npc_Behaviour_NodeHeaders.h"

#include <algorithm>
#include <format>
#include <../cereal/include/cereal/types/vector.hpp>

#include "../../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "../cereal/include/cereal/archives/portable_binary.hpp"

namespace Editor::Npc::Behaviour
{
    void RandomSelectorNode::OnDrawGraphEditorGui(
        const ImVec2& offset,
        ImDrawList* drawList,
        const std::weak_ptr<NodeBase>& ownPtr)
    {
        const Gui::Graph::NodeOption nodeOption
        {
            NODE_VISUAL_STYLE,
            "RandomSelector",
            true,
            false,
            NODE_SIZE
        };

        DrawGraphEditorGuiHelper::DrawNode(
            ownPtr, offset, PositionRef(), drawList, nodeOption, true);

        for (const auto& child : children_)
        {
            DrawGraphEditorGuiHelper::DrawNodePath(
                offset, *ownPtr.lock(), *child, drawList);

            child->OnDrawGraphEditorGui(offset, drawList, child);
        }
    }

    const std::string& RandomSelectorNode::NodeName() const
    {
        static const std::string NAME = "RandomSelectorNode";
        return NAME;
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus
    RandomSelectorNode::Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        if (children_.empty())
            return GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;

        // Running中で無いため、新しく randomIndex を選ぶ
        if (currentRunningNodeIndex_ < 0)
        {
            std::uniform_int_distribution dist(0, static_cast<int>(children_.size()) - 1);
            currentRunningNodeIndex_ = dist(rng_);
        }

        const auto& child = children_[currentRunningNodeIndex_];
        switch (const auto result = child->Tick(context))
        {
        case GameCore::Npc::Enemy::Behaviour::TickStatus::Running:
            return result;

        case GameCore::Npc::Enemy::Behaviour::TickStatus::Success:
        case GameCore::Npc::Enemy::Behaviour::TickStatus::Failure:
        case GameCore::Npc::Enemy::Behaviour::TickStatus::Abort:
            currentRunningNodeIndex_ = -1;
            return result;
        }

        currentRunningNodeIndex_ = -1;
        return GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus
    RandomSelectorNode::Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        if (children_.empty())
            return GameCore::Npc::Friendly::Behaviour::TickStatus::Failure;

        // Running中で無いため、新しく randomIndex を選ぶ
        if (currentRunningNodeIndex_ < 0)
        {
            std::uniform_int_distribution dist(0, static_cast<int>(children_.size()) - 1);
            currentRunningNodeIndex_ = dist(rng_);
        }

        const auto& child = children_[currentRunningNodeIndex_];
        switch (const auto result = child->Tick(context))
        {
        case GameCore::Npc::Friendly::Behaviour::TickStatus::Running:
            return result;

        case GameCore::Npc::Friendly::Behaviour::TickStatus::Success:
        case GameCore::Npc::Friendly::Behaviour::TickStatus::Failure:
        case GameCore::Npc::Friendly::Behaviour::TickStatus::Abort:
            currentRunningNodeIndex_ = -1;
            return result;
        }

        currentRunningNodeIndex_ = -1;
        return GameCore::Npc::Friendly::Behaviour::TickStatus::Failure;
    }

    void RandomSelectorNode::SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode)
    {
        children_.push_back(std::move(nextNode));
    }

    void RandomSelectorNode::DoOnDrawGui()
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
    void RandomSelectorNode::save(Archive& archive, const std::uint32_t) const
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(children_));
    }

    template<class Archive>
    void RandomSelectorNode::load(Archive& archive, const std::uint32_t)
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(children_));
    }

    template void RandomSelectorNode::save<cereal::JSONOutputArchive>( cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void RandomSelectorNode::load<cereal::JSONInputArchive>( cereal::JSONInputArchive&, const std::uint32_t);
    template void RandomSelectorNode::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void RandomSelectorNode::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}


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

    int RandomSelectorNode::PickWeightedIndex()
    {
        const bool anyPositive = std::any_of(weights_.begin(), weights_.end(), [](int w){ return w > 0; });
        if (!anyPositive)
        {
            std::uniform_int_distribution<int> uniform(0, static_cast<int>(children_.size()) - 1);
            return uniform(rng_);
        }
        std::discrete_distribution<int> dist(weights_.begin(), weights_.end());
        return dist(rng_);
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus
    RandomSelectorNode::Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        if (children_.empty())
            return GameCore::Npc::Enemy::Behaviour::TickStatus::Failure;

        if (currentRunningNodeIndex_ < 0)
            currentRunningNodeIndex_ = PickWeightedIndex();

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

        if (currentRunningNodeIndex_ < 0)
            currentRunningNodeIndex_ = PickWeightedIndex();

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
        weights_.push_back(100);
    }

    void RandomSelectorNode::DoOnDrawGui()
    {
        if (children_.empty())
        {
            ImGui::TextDisabled("No children");
            return;
        }

        int totalWeight = 0;
        for (int w : weights_) totalWeight += w;

        ImGui::Text("Children  (Total weight: %d)", totalWeight);
        ImGui::Separator();

        for (size_t i = 0; i < children_.size();)
        {
            ImGui::PushID(static_cast<int>(i));

            // 並び替えボタン
            if (ImGui::ArrowButton("Up", ImGuiDir_Up))
            {
                if (i > 0)
                {
                    std::swap(children_[i], children_[i - 1]);
                    std::swap(weights_[i], weights_[i - 1]);
                }
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("Down", ImGuiDir_Down))
            {
                if (i + 1 < children_.size())
                {
                    std::swap(children_[i], children_[i + 1]);
                    std::swap(weights_[i], weights_[i + 1]);
                }
            }
            ImGui::SameLine();

            // 重み入力
            ImGui::SetNextItemWidth(60.0f);
            ImGui::DragInt("##w", &weights_[i], 1.0f, 0, 100);
            ImGui::SameLine();

            // 実効確率表示
            const float prob = totalWeight > 0 ? weights_[i] * 100.0f / static_cast<float>(totalWeight) : 0.0f;
            ImGui::TextDisabled("(%.1f%%)", prob);
            ImGui::SameLine();

            // ラベル
            const std::string label = std::format("Child {} : {}", i, children_[i]->NodeName());
            ImGui::Selectable(label.c_str(), false);

            // 右クリックメニュー
            if (ImGui::BeginPopupContextItem("ChildContext"))
            {
                if (ImGui::MenuItem("Delete"))
                {
                    children_.erase(children_.begin() + i);
                    weights_.erase(weights_.begin() + i);
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
        archive(CEREAL_NVP(weights_));
    }

    template<class Archive>
    void RandomSelectorNode::load(Archive& archive, const std::uint32_t version)
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(children_));
        if (version >= 1)
        {
            archive(CEREAL_NVP(weights_));
        }
        else
        {
            weights_.assign(children_.size(), 100);
        }
    }

    template void RandomSelectorNode::save<cereal::JSONOutputArchive>( cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void RandomSelectorNode::load<cereal::JSONInputArchive>( cereal::JSONInputArchive&, const std::uint32_t);
    template void RandomSelectorNode::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void RandomSelectorNode::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}


#include "Npc_Behaviour_SelectorNode.h"

#include <algorithm>
#include <format>

#include "../Npc_Behaviour_NodeHeaders.h"
#include <../cereal/include/cereal/types/vector.hpp>

#include "../../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"
#include "../cereal/include/cereal/archives/json.hpp"
#include "../cereal/include/cereal/archives/portable_binary.hpp"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace Editor::Npc::Behaviour
{
    const std::string& SelectorNode::NodeName() const
    {
        static const std::string NAME = "SelectorNode";
        return NAME;
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus SelectorNode::DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
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

    GameCore::Npc::Friendly::Behaviour::TickStatus SelectorNode::DoTick(
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

    std::optional<ChildSlot> SelectorNode::RemoveChild(const NodeBase* child)
    {
        const auto it = std::ranges::find_if(children_, [child](const auto& c) { return c.get() == child; });
        if (it == children_.end())
            return std::nullopt;

        const auto index = static_cast<std::size_t>(it - children_.begin());
        children_.erase(it);
        return ChildSlot{ index };
    }

    void SelectorNode::InsertChild(std::shared_ptr<NodeBase> child, const ChildSlot& slot)
    {
        const std::size_t index = std::min(slot.index, children_.size());
        children_.insert(children_.begin() + static_cast<std::ptrdiff_t>(index), std::move(child));
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

            const std::string label = std::format("Child {} : {}", i, children_[i]->NodeName());

            ImGui::Selectable(label.c_str(), false);

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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(Editor::Npc::Behaviour::SelectorNode, Editor::Npc::Behaviour::NodeBase);
#pragma endregion

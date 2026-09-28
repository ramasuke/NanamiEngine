#pragma once
#include <memory>
#include <vector>
#include "../Npc_BehaviourNodeBase.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace Editor::Npc::Behaviour
{
    class EntryNode final : public NodeBase
    {
    public:
        explicit EntryNode();

        [[nodiscard]] const std::string& NodeName() const override;
        [[nodiscard]] std::string GraphNodeTitle() const override { return "Entry"; }
        [[nodiscard]] ImU32 GraphHeaderColor() const override { return IM_COL32(56, 150, 90, 255); }
        [[nodiscard]] std::size_t MaxChildren() const override { return 1; }
        std::optional<ChildSlot> RemoveChild(const NodeBase* child) override;
        [[nodiscard]] std::vector<std::shared_ptr<NodeBase>> Children() const override
        {
            if (nextNode_) return { nextNode_ };
            return {};
        }

    private:
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;
        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        std::shared_ptr<NodeBase> nextNode_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version);
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::EntryNode, 0);
#pragma endregion

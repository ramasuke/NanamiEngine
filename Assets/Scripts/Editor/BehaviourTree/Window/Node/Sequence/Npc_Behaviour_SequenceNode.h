#pragma once
#include <vector>
#include <memory>

#include "../Npc_BehaviourNodeBase.h"

namespace Editor::Npc::Behaviour
{
    class SequenceNode final : public NodeBase
    {
    public:
        [[nodiscard]] const std::string& NodeName() const override;
        [[nodiscard]] std::string GraphNodeTitle() const override { return "Sequence"; }
        [[nodiscard]] ImU32 GraphHeaderColor() const override { return IM_COL32(70, 140, 80, 255); }
        [[nodiscard]] std::size_t MaxChildren() const override { return UNLIMITED_CHILDREN; }
        std::optional<ChildSlot> RemoveChild(const NodeBase* child) override;
        void InsertChild(std::shared_ptr<NodeBase> child, const ChildSlot& slot) override;
        [[nodiscard]] std::vector<std::shared_ptr<NodeBase>> Children() const override { return children_; }

    private:
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;
        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        std::vector<std::shared_ptr<NodeBase>> children_;

#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, const std::uint32_t version);
#pragma endregion
    };

    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(SequenceNode)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::SequenceNode, 0);
#pragma endregion

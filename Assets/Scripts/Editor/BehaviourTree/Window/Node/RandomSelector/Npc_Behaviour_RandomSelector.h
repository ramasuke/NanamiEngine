#pragma once
#include <vector>
#include <memory>
#include <random>

#include "../Npc_BehaviourNodeBase.h"

namespace Editor::Npc::Behaviour
{
    class RandomSelectorNode final : public NodeBase
    {
    public:
        [[nodiscard]] const std::string& NodeName() const override;
        [[nodiscard]] std::string GraphNodeTitle() const override { return "RandomSelector"; }
        [[nodiscard]] std::string GraphNodeDetail() const override;
        [[nodiscard]] ImU32 GraphHeaderColor() const override { return IM_COL32(120, 150, 60, 255); }
        [[nodiscard]] std::size_t MaxChildren() const override { return UNLIMITED_CHILDREN; }
        std::optional<ChildSlot> RemoveChild(const NodeBase* child) override;
        void InsertChild(std::shared_ptr<NodeBase> child, const ChildSlot& slot) override;
        [[nodiscard]] std::vector<std::shared_ptr<NodeBase>> Children() const override { return children_; }

    private:
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus
        DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;

        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus
        DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        void DoResetRuntimeState() override;
        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        [[nodiscard]] int PickWeightedIndex();
        void PickNextChild();

        std::vector<std::shared_ptr<NodeBase>> children_;
        std::vector<int> weights_;
        std::mt19937 rng_{ std::random_device{}() };
        int currentRunningNodeIndex_ = -1;

#pragma region Serialization
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, const std::uint32_t version);
#pragma endregion
    };

    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(RandomSelectorNode)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::RandomSelectorNode, 1);
#pragma endregion

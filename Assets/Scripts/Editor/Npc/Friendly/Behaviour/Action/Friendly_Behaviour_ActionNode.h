#pragma once
#include <memory>

#include "../../../../BehaviourTree/Window/Node/Npc_BehaviourNodeBase.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    class ActionBase;
}

namespace Editor::Npc::Friendly::Behaviour
{
    class ActionNode final : public Npc::Behaviour::NodeBase
    {
    public:
        explicit ActionNode(std::unique_ptr<GameCore::Npc::Friendly::Behaviour::ActionBase> action = nullptr);
        ~ActionNode() override = default;
        [[nodiscard]] const std::string& NodeName() const override { return name_; }
        [[nodiscard]] std::string GraphNodeDetail() const override;
        [[nodiscard]] ImU32 GraphHeaderColor() const override { return IM_COL32(80, 90, 170, 255); }
        void DrawGraphContextMenuItems() override;
        
    private:
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;
        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        std::string name_;
        std::unique_ptr<GameCore::Npc::Friendly::Behaviour::ActionBase> action_;
        
#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, std::uint32_t version);
#pragma endregion
    };
    REGISTER_CREATABLE_ACTION_NODE_FACTORY(BehaviourTreeType::FriendlyNpc, ActionNode)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Friendly::Behaviour::ActionNode, 1);
#pragma endregion

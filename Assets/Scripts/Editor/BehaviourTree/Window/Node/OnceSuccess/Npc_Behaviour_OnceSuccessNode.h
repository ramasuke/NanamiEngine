#pragma once
#include <memory>
#include <vector>

#include "../Npc_BehaviourNodeBase.h"

namespace Editor::Npc::Behaviour
{
    class OnceSuccessNode final : public NodeBase
    {
    public:
        [[nodiscard]] const std::string& NodeName() const override;
        [[nodiscard]] ImU32 GraphHeaderColor() const override { return IM_COL32(40, 140, 150, 255); }
        [[nodiscard]] std::size_t MaxChildren() const override { return 1; }
        std::optional<ChildSlot> RemoveChild(const NodeBase* child) override;
        [[nodiscard]] std::vector<std::shared_ptr<NodeBase>> Children() const override
        {
            if (child_) return { child_ };
            return {};
        }

    private:
        [[nodiscard]] GameCore::Npc::Enemy   ::Behaviour::TickStatus DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

    private:
        std::shared_ptr<NodeBase> child_;
        bool hasSucceeded_ = false;

#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, const std::uint32_t version);
#pragma endregion
    };
    
    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(OnceSuccessNode)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::OnceSuccessNode, 0);
#pragma endregion

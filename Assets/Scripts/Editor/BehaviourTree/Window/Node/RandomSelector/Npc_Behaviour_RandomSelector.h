#pragma once
#include <vector>
#include <memory>
#include <random>

#include "../Npc_BehaviourNodeBase.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"

namespace Editor::Npc::Behaviour
{
    class RandomSelectorNode final : public NodeBase
    {
    public:
        void OnDrawGraphEditorGui(
            const ImVec2& offset,
            ImDrawList* drawList,
            const std::weak_ptr<NodeBase>& ownPtr) override;
        [[nodiscard]] const std::string& NodeName() const override;

    private:
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus
        Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;

        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus
        Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        std::vector<std::shared_ptr<NodeBase>> children_;
        std::mt19937 rng_{ std::random_device{}() };
        int currentRunningNodeIndex_ = -1;

        inline static const auto NODE_VISUAL_STYLE =
            Gui::Graph::NodeVisualStyle(
                IM_COL32(50 , 70 , 50 , 255),
                IM_COL32(200, 200, 200, 255),
                IM_COL32(180, 220, 120, 255),
                IM_COL32_WHITE
            );

#pragma region Serialization
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, const std::uint32_t version);
#pragma endregion
    };

    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(RandomSelectorNode)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::RandomSelectorNode, 0);
CEREAL_REGISTER_TYPE(Editor::Npc::Behaviour::RandomSelectorNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    Editor::Npc::Behaviour::NodeBase,
    Editor::Npc::Behaviour::RandomSelectorNode
);
#pragma endregion

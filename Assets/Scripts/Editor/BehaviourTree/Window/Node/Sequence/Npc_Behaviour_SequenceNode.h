#pragma once
#include <vector>
#include <memory>

#include "../Npc_BehaviourNodeBase.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"

namespace Editor::Npc::Behaviour
{
    class SequenceNode final : public NodeBase
    {
    public:
        void OnDrawGraphEditorGui(
            const ImVec2& offset,
            ImDrawList* drawList,
            const std::weak_ptr<NodeBase>& ownPtr) override;

        [[nodiscard]] const std::string& NodeName() const override { return "SequenceNode"; }
        
    private:
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;
        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        std::vector<std::shared_ptr<NodeBase>> children_;

        inline static const auto NODE_VISUAL_STYLE =
            Gui::Graph::NodeVisualStyle(
                IM_COL32(50 , 70 , 50 , 255),
                IM_COL32(200, 200, 200, 255),
                IM_COL32(180, 180, 100, 255),
                IM_COL32_WHITE
            );

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
CEREAL_REGISTER_TYPE(Editor::Npc::Behaviour::SequenceNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Editor::Npc::Behaviour::NodeBase, Editor::Npc::Behaviour::SequenceNode);
#pragma endregion

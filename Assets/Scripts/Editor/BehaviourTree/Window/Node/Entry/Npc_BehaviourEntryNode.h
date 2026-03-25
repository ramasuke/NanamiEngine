#pragma once
#include <memory>
#include "../Npc_BehaviourNodeBase.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"
#include "../../../Engine/Module/Namespace/EngineNamespace.h"

namespace Editor::Npc::Behaviour
{
    class EntryNode final : public NodeBase
    {
    public:
        explicit EntryNode();

        GameCore::Npc::Enemy::Behaviour::TickStatus Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        GameCore::Npc::Friendly::Behaviour::TickStatus Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        [[nodiscard]] const std::string& NodeName() const override { return "EntryNode"; }
        void OnDrawGraphEditorGui(const ImVec2& offset, ImDrawList* drawList, const std::weak_ptr<NodeBase>& ownPtr) override;

    private:
        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        std::shared_ptr<NodeBase> nextNode_;

        inline static const auto NODE_VISUAL_STYLE = Gui::Graph::NodeVisualStyle
        (
            IM_COL32(50 , 50 , 70 , 255),
            IM_COL32(200, 200, 200, 255),
            IM_COL32(180, 180, 100, 255),
            IM_COL32_WHITE
        );
        inline static const auto ENTRY_NODE_NAME = "Entry";

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
CEREAL_REGISTER_TYPE(Editor::Npc::Behaviour::EntryNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Editor::Npc::Behaviour::NodeBase, Editor::Npc::Behaviour::EntryNode);
#pragma endregion

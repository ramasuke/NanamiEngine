#pragma once
#include <memory>

#include "../Npc_BehaviourNodeBase.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"

namespace Editor::Npc::Behaviour
{
    class OnceSuccessNode final : public NodeBase
    {
    public:
        [[nodiscard]] const std::string& NodeName() const override;

    private:
        [[nodiscard]] GameCore::Npc::Enemy   ::Behaviour::TickStatus Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void OnDrawGraphEditorGui(
            const ImVec2& offset,
            ImDrawList* drawList,
            const std::weak_ptr<NodeBase>& ownPtr) override;
        void DoOnDrawGui() override;

    private:
        std::shared_ptr<NodeBase> child_;
        bool hasSucceeded_ = false;
        
        inline static const auto NODE_VISUAL_STYLE =
            Gui::Graph::NodeVisualStyle(
                IM_COL32(0 , 70 , 70 , 255),
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
    
    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(OnceSuccessNode)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::OnceSuccessNode, 0);
CEREAL_REGISTER_TYPE(Editor::Npc::Behaviour::OnceSuccessNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Editor::Npc::Behaviour::NodeBase, Editor::Npc::Behaviour::OnceSuccessNode);
#pragma endregion


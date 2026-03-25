#pragma once
#include <memory>

#include "../../../../BehaviourTree/Window/Node/Npc_BehaviourNodeBase.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"
#include "../../../../../Core/Game/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionBase.h"

namespace Editor::Npc::Enemy::Behaviour
{
    class ActionNode final : public Npc::Behaviour::NodeBase
    {
    public:
        explicit ActionNode(std::unique_ptr<GameCore::Npc::Enemy::Behaviour::ActionBase> action = nullptr);
        ~ActionNode() override = default;
        void OnDrawGraphEditorGui(const ImVec2& offset, ImDrawList* drawList, const std::weak_ptr<NodeBase>& ownPtr) override;
        [[nodiscard]] const std::string& NodeName() const override { return name_; }
        
    private:
        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;
        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;
        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        void DoOnDrawGui() override;

        std::string name_;
        std::unique_ptr<GameCore::Npc::Enemy::Behaviour::ActionBase> action_;
        
        inline static const auto NODE_VISUAL_STYLE = Gui::Graph::NodeVisualStyle
        (
            IM_COL32(50 , 50 , 70 , 255),
            IM_COL32(200, 200, 200, 255),
            IM_COL32(180, 180, 100, 255),
            IM_COL32_WHITE
        );

#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, std::uint32_t version);
#pragma endregion
    };
    REGISTER_CREATABLE_ACTION_NODE_FACTORY(BehaviourTreeType::EnemyNpc, ActionNode)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Enemy::Behaviour::ActionNode, 1);
CEREAL_REGISTER_TYPE(Editor::Npc::Enemy::Behaviour::ActionNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Editor::Npc::Behaviour::NodeBase, Editor::Npc::Enemy::Behaviour::ActionNode);
#pragma endregion

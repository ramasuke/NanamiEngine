#pragma once
#include <memory>

#include "../Npc_BehaviourNodeBase.h"
#include "../../../../../../../Engine/Module/Gui/Graph/NodeOption/VisualStyle/NodeVisualStyle.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"
#include "../../../../../Core/Game/Npc/Enemy/Behaviour/TickStatus/TickStatus.h"

namespace Editor::Npc::Behaviour
{
    class OnceExecute final : public NodeBase
    {
    public:
        [[nodiscard]] const std::string& NodeName() const override;

    private:
        enum class State
        {
            NotExecuted,
            Running,
            Success,
            Failure
        };

        std::shared_ptr<NodeBase> child_;
        State state_ = State::NotExecuted;

        inline static const auto NODE_VISUAL_STYLE =
            Gui::Graph::NodeVisualStyle(
                IM_COL32(70 , 70 , 0 , 255),    // 背景（落ち着いた黄色系）
                IM_COL32(200, 200, 200, 255),   // 枠
                IM_COL32(180, 180, 100, 255),   // ヘッダー
                IM_COL32_WHITE                  // テキスト
            );
        
    private:
        template<class Context, class TickStatus>
        TickStatus TickImpl(const Context& context)
        {
            if (!child_)
                return TickStatus::Failure;

            if (state_ == State::Success)
                return TickStatus::Success;
            if (state_ == State::Failure)
                return TickStatus::Failure;

            const auto result = child_->Tick(context);

            if (result == TickStatus::Running)
            {
                state_ = State::Running;
                return result;
            }

            if (result == TickStatus::Success)
            {
                state_ = State::Success;
                return result;
            }

            state_ = State::Failure;
            return result;
        }

        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus
        Tick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;

        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus
        Tick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;
        
#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, const std::uint32_t version);
        void OnDrawGraphEditorGui(const ImVec2& offset, ImDrawList* drawList,
            const std::weak_ptr<NodeBase>& ownPtr) override;

    private:
        void DoOnDrawGui() override;

    public:
#pragma endregion
    };
    
    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(OnceExecute)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::OnceExecute, 0);
CEREAL_REGISTER_TYPE(Editor::Npc::Behaviour::OnceExecute);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Editor::Npc::Behaviour::NodeBase, Editor::Npc::Behaviour::OnceExecute);
#pragma endregion
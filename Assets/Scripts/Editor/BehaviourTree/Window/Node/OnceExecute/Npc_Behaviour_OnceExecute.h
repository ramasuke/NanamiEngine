#pragma once
#include <memory>
#include <vector>

#include "../Npc_BehaviourNodeBase.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/TickStatus/Friendly_Behaviour_TickStatus.h"
#include "../../../../../Core/Game/Npc/Enemy/Behaviour/TickStatus/TickStatus.h"

namespace Editor::Npc::Behaviour
{
    class OnceExecute final : public NodeBase
    {
    public:
        [[nodiscard]] const std::string& NodeName() const override;
        [[nodiscard]] ImU32 GraphHeaderColor() const override { return IM_COL32(160, 150, 40, 255); }
        [[nodiscard]] std::size_t MaxChildren() const override { return 1; }
        std::optional<ChildSlot> RemoveChild(const NodeBase* child) override;
        [[nodiscard]] std::vector<std::shared_ptr<NodeBase>> Children() const override
        {
            if (child_) return { child_ };
            return {};
        }

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
        DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;

        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus
        DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;

        // NOTE: RandomSelector が枝を選び直すたびに、もう一度 1 回だけ実行できるように戻す
        void DoResetRuntimeState() override { state_ = State::NotExecuted; }

#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const;
        template<class Archive> void load(Archive& archive, const std::uint32_t version);

    private:
        void DoOnDrawGui() override;

    public:
#pragma endregion
    };
    
    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(OnceExecute)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::OnceExecute, 0);
#pragma endregion

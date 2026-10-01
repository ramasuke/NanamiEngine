#pragma once
#include <memory>
#include <string>
#include <vector>

#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

#include "../Npc_BehaviourNodeBase.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace Editor::Npc::Behaviour
{
    /** @brief ブラックボードの int キーと値の組 */
    struct BlackBoardIntEntry
    {
        std::string keyName_;
        int value_ = 0;

        void OnDrawGui()
        {
            LibCore::ImGuiHelper::OnDrawInputField("keyName_", keyName_);
            LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
        }

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(CEREAL_NVP(keyName_));
            archive(CEREAL_NVP(value_));
        }
    };

    /** @brief ブラックボードの条件がすべて一致したときだけ子を実行する。子が無ければ判定だけで Success */
    class BlackBoardGate final : public NodeBase
    {
    public:
        [[nodiscard]] const std::string& NodeName() const override;
        [[nodiscard]] std::string GraphNodeDetail() const override;
        [[nodiscard]] ImU32 GraphHeaderColor() const override { return IM_COL32(60, 110, 160, 255); }
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
        /** @brief すべて一致で子を実行。1 つでも違えば Failure */
        std::vector<BlackBoardIntEntry> conditions_;
        /**
         * @brief 子を実行し始めるときに書く
         * NOTE: 条件は毎 Tick 判定するので、条件のキーを書き換えると次の Tick で外れる
         */
        std::vector<BlackBoardIntEntry> writesOnStart_;
        /** @brief 子が Success を返したときに書く */
        std::vector<BlackBoardIntEntry> writesOnSuccess_;
        /** @brief OnceExecute と同じく、終わった後は結果を返し続ける */
        bool once_ = false;

        State state_ = State::NotExecuted;
        std::uint64_t lastTickIndex_ = 0;

        template<class Context, class TickStatus>
        TickStatus TickImpl(const Context& context);

        [[nodiscard]] bool MatchesConditions(const NanamiEngine::Module::BlackBoard::ParameterGroup& parameters) const;
        static void Write(const NanamiEngine::Module::BlackBoard::ParameterGroup& parameters,
                          const std::vector<BlackBoardIntEntry>& entries);

        [[nodiscard]] GameCore::Npc::Enemy::Behaviour::TickStatus
        DoTick(const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context) override;

        [[nodiscard]] GameCore::Npc::Friendly::Behaviour::TickStatus
        DoTick(const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context) override;

        void SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode) override;

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

    REGISTER_CREATABLE_BEHAVIOUR_NODE_FACTORY(BlackBoardGate)
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Editor::Npc::Behaviour::BlackBoardGate, 0);
#pragma endregion

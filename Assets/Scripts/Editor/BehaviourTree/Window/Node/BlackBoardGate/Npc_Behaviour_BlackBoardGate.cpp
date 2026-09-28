#include "Npc_Behaviour_BlackBoardGate.h"

#include <format>

#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/Action/TickContext/Friendly_Behaviour_TickContext.h"

namespace Editor::Npc::Behaviour
{
    namespace
    {
        void DrawEntries(const char* label, std::vector<BlackBoardIntEntry>& entries)
        {
            LibCore::ImGuiHelper::OnDrawInputField(label, entries, [&entries]
            {
                if (ImGui::Button("Add"))
                    entries.emplace_back();
            });
        }

        std::string FormatEntries(const std::vector<BlackBoardIntEntry>& entries, const char* op)
        {
            std::string text;
            for (const auto& entry : entries)
            {
                if (!text.empty()) text += ", ";
                text += std::format("{}{}{}", entry.keyName_, op, entry.value_);
            }
            return text;
        }
    }

    const std::string& BlackBoardGate::NodeName() const
    {
        static const std::string NAME = "BlackBoardGate";
        return NAME;
    }

    std::string BlackBoardGate::GraphNodeDetail() const
    {
        std::string detail = FormatEntries(conditions_, "==");
        if (!writesOnStart_.empty())   detail += "\nstart: " + FormatEntries(writesOnStart_, "=");
        if (!writesOnSuccess_.empty()) detail += "\nok: " + FormatEntries(writesOnSuccess_, "=");
        if (once_)                     detail += "\nonce";
        return detail;
    }

    bool BlackBoardGate::MatchesConditions(const NanamiEngine::Module::BlackBoard::ParameterGroup& parameters) const
    {
        for (const auto& condition : conditions_)
        {
            const auto param = parameters.Catch<int>(condition.keyName_);
            if (!param || param->Get() != condition.value_)
                return false;
        }
        return true;
    }

    void BlackBoardGate::Write(const NanamiEngine::Module::BlackBoard::ParameterGroup& parameters,
                               const std::vector<BlackBoardIntEntry>& entries)
    {
        for (const auto& entry : entries)
        {
            if (const auto param = parameters.Catch<int>(entry.keyName_))
                param->Set(entry.value_);
        }
    }

    template<class Context, class TickStatus>
    TickStatus BlackBoardGate::TickImpl(const Context& context)
    {
        if (once_ && state_ == State::Success) return TickStatus::Success;
        if (once_ && state_ == State::Failure) return TickStatus::Failure;

        // NOTE: Running のまま前回の Tick で呼ばれなかった = 割り込まれたので、次は最初から入り直す
        if constexpr (requires { context.TickIndex(); })
        {
            if (state_ == State::Running && lastTickIndex_ + 1 != context.TickIndex())
                state_ = State::NotExecuted;
            lastTickIndex_ = context.TickIndex();
        }

        const auto& parameters = *context.Parameter();
        if (!MatchesConditions(parameters))
        {
            if (!once_) state_ = State::NotExecuted;
            return TickStatus::Failure;
        }

        if (state_ != State::Running)
            Write(parameters, writesOnStart_);

        const TickStatus result = child_ ? child_->Tick(context) : TickStatus::Success;

        switch (result)
        {
        case TickStatus::Running:
            state_ = State::Running;
            break;
        case TickStatus::Success:
            Write(parameters, writesOnSuccess_);
            state_ = once_ ? State::Success : State::NotExecuted;
            break;
        default:
            state_ = once_ ? State::Failure : State::NotExecuted;
            break;
        }
        return result;
    }

    GameCore::Npc::Enemy::Behaviour::TickStatus BlackBoardGate::DoTick(
        const GameCore::Npc::Enemy::Behaviour::Action::TickContext& context)
    {
        return TickImpl<
            GameCore::Npc::Enemy::Behaviour::Action::TickContext,
            GameCore::Npc::Enemy::Behaviour::TickStatus>(context);
    }

    GameCore::Npc::Friendly::Behaviour::TickStatus BlackBoardGate::DoTick(
        const GameCore::Npc::Friendly::Behaviour::Action::TickContext& context)
    {
        return TickImpl<
            GameCore::Npc::Friendly::Behaviour::Action::TickContext,
            GameCore::Npc::Friendly::Behaviour::TickStatus>(context);
    }

    void BlackBoardGate::SetConnectToNextNode(std::shared_ptr<NodeBase> nextNode)
    {
        child_ = std::move(nextNode);
    }

    std::optional<ChildSlot> BlackBoardGate::RemoveChild(const NodeBase* child)
    {
        if (!child_ || child_.get() != child)
            return std::nullopt;

        child_.reset();
        return ChildSlot{};
    }

    void BlackBoardGate::DoOnDrawGui()
    {
        ImGui::Text("BlackBoard Gate");
        ImGui::Separator();
        DrawEntries("conditions_", conditions_);
        DrawEntries("writesOnStart_", writesOnStart_);
        DrawEntries("writesOnSuccess_", writesOnSuccess_);
        LibCore::ImGuiHelper::OnDrawInputField("once_", once_);

        if (!child_)
            ImGui::TextDisabled("No Child Node (condition only)");
        else
            ImGui::Text("Child : %s", child_->NodeName().c_str());
    }

    template <class Archive>
    void BlackBoardGate::save(Archive& archive, const std::uint32_t version) const
    {
        archive(cereal::base_class<NodeBase>(this));
        archive(CEREAL_NVP(child_));
        archive(CEREAL_NVP(conditions_));
        archive(CEREAL_NVP(writesOnStart_));
        archive(CEREAL_NVP(writesOnSuccess_));
        archive(CEREAL_NVP(once_));
    }

    template <class Archive>
    void BlackBoardGate::load(Archive& archive, const std::uint32_t version)
    {
        archive(cereal::base_class<NodeBase>(this));
        if (version >= 0) archive(CEREAL_NVP(child_));
        if (version >= 0) archive(CEREAL_NVP(conditions_));
        if (version >= 0) archive(CEREAL_NVP(writesOnStart_));
        if (version >= 0) archive(CEREAL_NVP(writesOnSuccess_));
        if (version >= 0) archive(CEREAL_NVP(once_));
    }

    template void BlackBoardGate::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
    template void BlackBoardGate::load<cereal::JSONInputArchive >(cereal::JSONInputArchive&, const std::uint32_t);
    template void BlackBoardGate::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
    template void BlackBoardGate::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(Editor::Npc::Behaviour::BlackBoardGate, Editor::Npc::Behaviour::NodeBase);
#pragma endregion

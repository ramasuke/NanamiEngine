#include "Enemy_Behaviour_Action_ActionTimeline.h"

#include <format>
#include <typeinfo>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Gui/StaticReflection/Engine_Module_StaticReflection.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    namespace
    {
        std::string ActionTypeName(const ActionBase* action)
        {
            if (!action)
                return "(no action)";

            // NOTE: typeid の名前は "class A::B::Name" なので、最後の型名だけを出す
            const std::string typeName = typeid(*action).name();
            const auto separator = typeName.find_last_of(": ");
            return separator == std::string::npos ? typeName : typeName.substr(separator + 1);
        }
    }

    TickStatus ActionTimeline::DoTick(const TickContext& context)
    {
        // NOTE: WaitSeconds と同じく、終わった直後のフレームも続けて Tick されている間は完了のまま（Sequence の途中に置ける）
        const bool consecutive = lastTickIndex_ + 1 == context.TickIndex();
        lastTickIndex_ = context.TickIndex();
        if (completed_)
        {
            if (once_)
                return TickStatus::Success;
            if (consecutive)
            {
                for (auto& cue : cues_)
                {
                    if (cue.keepTicking_ && cue.action_)
                        (void)cue.action_->Tick(context);
                }
                return TickStatus::Success;
            }
            completed_ = false;
            ResetCues();
        }

        if (cueStates_.size() != cues_.size())
            cueStates_.assign(cues_.size(), CueState::Pending);

        bool waitingCue = false;
        for (std::size_t i = 0; i < cues_.size(); ++i)
        {
            auto& cue = cues_[i];
            auto& state = cueStates_[i];

            if (state == CueState::Pending)
            {
                if (elapsed_secs_ < cue.at_secs_)
                {
                    waitingCue = true;
                    continue;
                }
                state = CueState::Running;
            }

            if (!cue.action_)
            {
                state = CueState::Done;
                continue;
            }

            if (state == CueState::Done)
            {
                if (cue.keepTicking_)
                    (void)cue.action_->Tick(context);
                continue;
            }

            switch (cue.action_->Tick(context))
            {
            case TickStatus::Running:
                if (cue.waitDone_) waitingCue = true;
                break;
            case TickStatus::Abort:
                return TickStatus::Abort;
            case TickStatus::Failure:
                state = CueState::Done;
                if (failOnChildFailure_) return TickStatus::Failure;
                break;
            case TickStatus::Success:
                state = CueState::Done;
                break;
            }
        }

        const bool finished = !waitingCue && duration_secs_ <= elapsed_secs_;
        elapsed_secs_ += Time::DeltaTime();

        if (!finished)
            return TickStatus::Running;

        completed_ = true;
        return TickStatus::Success;
    }

    void ActionTimeline::DoReset()
    {
        completed_ = false;
        ResetCues();
    }

    void ActionTimeline::ResetCues()
    {
        elapsed_secs_ = 0.0f;
        cueStates_.assign(cues_.size(), CueState::Pending);
        for (auto& cue : cues_)
        {
            if (cue.action_)
                cue.action_->Reset();
        }
    }

    void ActionTimeline::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("duration_secs_", duration_secs_);
        ImGuiHelper::OnDrawInputField("failOnChildFailure_", failOnChildFailure_);
        ImGuiHelper::OnDrawInputField("once_", once_);
        ImGui::Text("elapsed : %.2f", elapsed_secs_);
        ImGui::Separator();

        int eraseIndex = -1;
        for (std::size_t i = 0; i < cues_.size(); ++i)
        {
            auto& cue = cues_[i];
            ImGui::PushID(static_cast<int>(i));

            if (ImGui::ArrowButton("Up", ImGuiDir_Up) && i > 0)
                std::swap(cues_[i], cues_[i - 1]);
            ImGui::SameLine();
            if (ImGui::ArrowButton("Down", ImGuiDir_Down) && i + 1 < cues_.size())
                std::swap(cues_[i], cues_[i + 1]);
            ImGui::SameLine();
            if (ImGui::Button("Delete"))
                eraseIndex = static_cast<int>(i);
            ImGui::SameLine();

            const std::string label = std::format("{:.2f}s {}###Cue", cue.at_secs_, ActionTypeName(cue.action_.get()));
            if (ImGui::TreeNode(label.c_str()))
            {
                ImGuiHelper::OnDrawInputField("at_secs_", cue.at_secs_);
                ImGuiHelper::OnDrawInputField("waitDone_", cue.waitDone_);
                ImGuiHelper::OnDrawInputField("keepTicking_", cue.keepTicking_);

                if (ImGui::Button("Action..."))
                    ImGui::OpenPopup("CueAction");
                if (ImGui::BeginPopup("CueAction"))
                {
                    const auto& actions = Editor::Npc::Enemy::Behaviour::ActionFactory::Instance().CreatableActions();
                    auto tree = NanamiEngine::Module::StaticReflection::BuildTree<ActionBase>(actions);
                    NanamiEngine::Module::StaticReflection::DrawTreeGui(tree, cue.action_);
                    ImGui::EndPopup();
                }

                if (cue.action_)
                    cue.action_->OnDrawGui();

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if (eraseIndex >= 0)
            cues_.erase(cues_.begin() + eraseIndex);

        if (ImGui::Button("Add Cue"))
        {
            Cue cue;
            cue.at_secs_ = cues_.empty() ? 0.0f : cues_.back().at_secs_;
            cues_.push_back(std::move(cue));
        }

        // NOTE: 編集で並びや数が変わったので、実行時状態は作り直す
        if (cueStates_.size() != cues_.size())
            cueStates_.clear();
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ActionTimeline, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion

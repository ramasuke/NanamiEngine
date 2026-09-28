#include "Enemy_Behaviour_Action_RandomWriteBlackBoard.h"

#include <algorithm>
#include <random>

#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    void RandomWriteBlackBoard::Choice::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("value_", value_);
        ImGuiHelper::OnDrawInputField("weight_", weight_);
    }

    TickStatus RandomWriteBlackBoard::DoTick(const TickContext& context)
    {
        const auto param = context.Parameter()->Catch<int>(keyName_);
        const bool anyPositive = std::ranges::any_of(choices_, [](const Choice& c) { return c.weight_ > 0; });
        if (!param || !anyPositive)
            return TickStatus::Failure;

        std::vector<int> weights;
        weights.reserve(choices_.size());
        for (const auto& choice : choices_)
            weights.push_back(std::max(choice.weight_, 0));

        static std::mt19937 rng{ std::random_device{}() };
        std::discrete_distribution<int> dist(weights.begin(), weights.end());
        param->Set(choices_[dist(rng)].value_);
        return TickStatus::Success;
    }

    void RandomWriteBlackBoard::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("keyName_", keyName_);
        ImGuiHelper::OnDrawInputField("choices_", choices_, [this]
        {
            if (ImGui::Button("Add"))
                choices_.emplace_back();
        });
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::RandomWriteBlackBoard, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion

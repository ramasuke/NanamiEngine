#include "Enemy_Behaviour_Action_ChangeColliderEmotionType.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeColliderEmotionType::DoTick(const TickContext& context)
    {
        for (const auto& rigidBody : colliders_)
        {
            rigidBody.get(context)->SetMotionType(emotionType_);
        }

        return TickStatus::Success;
    }

    void Action::ChangeColliderEmotionType::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rigidBodies", colliders_, [this]()
        {
            if (ImGui::Button("Add"))
                colliders_.emplace_back();
        });

        Physics::DrawChoiceMotionTypeGui("Motion Type", emotionType_);
    }
}

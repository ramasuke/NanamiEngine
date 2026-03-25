#include "Enemy_Behaviour_Action_ChangeColliderEmotionType.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeColliderEmotionType::DoTick(const TickContext& context)
    {
        for (const auto& collider : colliders_)
        {
            collider->ChangeEmotionType(emotionType_);
        }
        
        return TickStatus::Success;
    }

    void Action::ChangeColliderEmotionType::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("colliders", colliders_, [this]()
        {
            if (ImGui::Button("Add"))
                colliders_.emplace_back();
        });
        
        static const char* motionTypeNames[] = {
            "Static", "Kinematic", "Dynamic"
        };

        int currentIndex = static_cast<int>(emotionType_);
        if (ImGui::Combo("Motion Type", &currentIndex, motionTypeNames, IM_ARRAYSIZE(motionTypeNames)))
        {
            emotionType_ = static_cast<JPH::EMotionType>(currentIndex);
        }
    }
}

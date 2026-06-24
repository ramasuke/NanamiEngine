#include "Enemy_Behaviour_Action_Position.h"

#include "../TickContext/Enemy_Behaviour_TickContext.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    glm::vec3 Position::get(const TickContext& context) const
    {
        switch (mode_)
        {
        case Mode::AbsolutePosition:
            return offset_;

        case Mode::TargetObject:
            {
                return targetObject_->Transform().GetWorldPos() + offset_;
            }

        case Mode::EnemyOffset:
        default:
            {
                const auto enemyPos = context.EnemyTransform().GetWorldPos();
                const auto enemyRot = context.EnemyTransform().GetWorldRot();
                const auto localRot = glm::quat(glm::radians(offsetRotation_));
                return enemyPos + enemyRot * (localRot * offset_);
            }
        }
    }

    void Position::OnDrawGui()
    {
        const char* items[] = {"EnemyOffset", "AbsolutePosition", "TargetObject"};
        int current = static_cast<int>(mode_);
        if (ImGui::Combo("Mode", &current, items, IM_ARRAYSIZE(items)))
        {
            mode_ = static_cast<Mode>(current);
        }

        ImGuiHelper::OnDrawInputField("offset_", offset_);
        if (mode_ == Mode::EnemyOffset)
        {
            ImGuiHelper::OnDrawInputField("offsetRotation_", offsetRotation_);
        }
        if (mode_ == Mode::TargetObject)
        {
            ImGuiHelper::OnDrawInputField("targetObject_", targetObject_);
        }
    }
}

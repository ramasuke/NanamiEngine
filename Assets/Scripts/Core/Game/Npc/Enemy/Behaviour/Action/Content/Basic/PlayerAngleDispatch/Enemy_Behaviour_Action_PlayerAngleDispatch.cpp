#include "Enemy_Behaviour_Action_PlayerAngleDispatch.h"

#include <cmath>

#include "Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../Common/Enemy_Behaviour_FaceDirection.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    void PlayerAngleDispatch::Range::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("minDegree_", minDegree_);
        ImGuiHelper::OnDrawInputField("maxDegree_", maxDegree_);
        ImGuiHelper::OnDrawInputField("useAbsolute_", useAbsolute_);
        ImGuiHelper::OnDrawInputField("value_", value_);
    }

    TickStatus PlayerAngleDispatch::DoTick(const TickContext& context)
    {
        const auto param = context.Parameter()->Catch<int>(keyName_);
        if (!param)
            return TickStatus::Failure;

        const auto& transform = context.EnemyTransform();
        glm::vec3 playerPos;
        float angle = 0.0f;
        const bool hasAngle = context.NearestPlayerPosition(transform.GetWorldPos(), playerPos)
                           && SignedHorizontalAngleDeg(transform, playerPos - transform.GetWorldPos(), angle);

        if (hasAngle)
        {
            for (const auto& range : ranges_)
            {
                const float a = range.useAbsolute_ ? std::abs(angle) : angle;
                if (range.minDegree_ <= a && a <= range.maxDegree_)
                {
                    param->Set(range.value_);
                    return TickStatus::Success;
                }
            }
        }

        if (!useFallback_)
            return TickStatus::Failure;

        param->Set(fallbackValue_);
        return TickStatus::Success;
    }

    void PlayerAngleDispatch::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("keyName_", keyName_);
        ImGuiHelper::OnDrawInputField("ranges_", ranges_, [this]
        {
            if (ImGui::Button("Add"))
                ranges_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("useFallback_", useFallback_);
        ImGuiHelper::OnDrawInputField("fallbackValue_", fallbackValue_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PlayerAngleDispatch, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion

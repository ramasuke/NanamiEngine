#include "Enemy_Behaviour_Action_RotateOnPlayer.h"

#include "../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../glm/gtx/quaternion.hpp"
#include "../glm/gtx/vector_angle.hpp"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::RotateOnPlayer::DoTick(const TickContext& context)
    {
        auto& transform = context.EnemyTransform();
        
        // Player への方向
        glm::vec3 targetDir = context.Player().PlayerTransform().GetWorldPos() - transform.GetWorldPos();
        targetDir.y = 0.0f;
        if (glm::length2(targetDir) < 0.0001f)
            return TickStatus::Failure;

        targetDir = glm::normalize(targetDir);

        // Enemy forward（モデル前方向は -Z）
        glm::vec3 currentForward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
        currentForward.y = 0.0f;

        if (glm::length2(currentForward) < 0.0001f)
            return TickStatus::Failure;

        currentForward = glm::normalize(currentForward);

        const float dot = glm::clamp(glm::dot(currentForward, targetDir), -1.0f, 1.0f);
        const float angleDeg = glm::degrees(std::acos(dot));

        // もう十分向いている → 回る必要なし
        if (angleDeg <= toleranceDeg_)
            return TickStatus::Failure;

        const glm::quat currentRot = transform.GetWorldRot();

        // 正反対（180度）対策
        if (dot < -0.9999f)
        {
            const glm::quat deltaRot = glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0));

            const float t = glm::min(1.0f, glm::radians(rotateSpeed_) * Time::DeltaTime() / glm::pi<float>());
            transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
            return TickStatus::Running;
        }

        // 通常回転
        const glm::quat deltaRot = glm::rotation(currentForward, targetDir);
        const float angleRad = glm::angle(deltaRot);

        const float maxStep = glm::radians(rotateSpeed_) * Time::DeltaTime();
        const float t = glm::min(1.0f, maxStep / angleRad);
        transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));

        return TickStatus::Running;
    }

    void Action::RotateOnPlayer::DoDrawGui()
    {
        ImGui::DragFloat("Rotate Speed (deg/sec)", &rotateSpeed_, 1.0f, 1.0f, 720.0f);
        ImGui::DragFloat("Tolerance (deg)", &toleranceDeg_, 0.1f, 0.0f, 45.0f);
        ImGui::DragFloat("Offset Rotate (deg)", &offsetRotateDeg_, 0.1f, -180.0f, 180.0f);

        if (toleranceDeg_ <= 0.0f)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Tolerance <= 0 : Always Failure");
        }
    }
}

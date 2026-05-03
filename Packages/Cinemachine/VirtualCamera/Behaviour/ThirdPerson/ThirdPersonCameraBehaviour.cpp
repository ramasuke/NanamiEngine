#include "ThirdPersonCameraBehaviour.h"
#include "../../../Brain/CinemachineCameraBrain.h"
#include "DxLib.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "gtx/rotate_vector.hpp"

namespace
{
    constexpr float STICK_MAX       = 32767.0f;
    constexpr int   STICK_DEAD_ZONE = 8000;
}

namespace NanamiEngine::CineMachine::Behaviour
{
    void ThirdPersonCameraBehaviour::SetTarget(const std::shared_ptr<GameObject::IGameObject>& target)
    {
        target_ = target;
        RequireComponent<VirtualCameraFollowBehaviour>()->SetTarget(target);
        RequireComponent<VirtualCameraLookAtBehaviour>()->SetTarget(target);
    }

    void ThirdPersonCameraBehaviour::SetLookAtOffsetPos(const glm::vec3& offsetPos)
    {
        lookAtOffsetPos_ = offsetPos;
        lookAt_->SetOffsetPos(lookAtOffsetPos_);
    }

    void ThirdPersonCameraBehaviour::OnAwake()
    {
        follow_ = RequireComponent<VirtualCameraFollowBehaviour>();
        lookAt_ = RequireComponent<VirtualCameraLookAtBehaviour>();
    }

    void ThirdPersonCameraBehaviour::OnUpdate()
    {
        if (!target_)
            return;

        UpdateMouseInput();
        UpdateGamepadInput();

        pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);

        UpdateFollowTargetBehaviour();
        UpdateLookAtTargetBehaviour();
    }

    void ThirdPersonCameraBehaviour::MainCameraCallback()
    {
        // cameraBrain_->TransformRef().SetWorldMatrix(Transform().GetWorldMatrix());
    }

    void ThirdPersonCameraBehaviour::UpdateMouseInput()
    {
        int mx, my;
        GetMousePoint(&mx, &my);

        static int cx = 640;
        static int cy = 360;

        const int dx = mx - cx;
        const int dy = my - cy;

        SetMousePoint(cx, cy);

        yaw_   += dx * mouseSensitivity_;
        pitch_ += dy * mouseSensitivity_;
    }

    void ThirdPersonCameraBehaviour::UpdateGamepadInput()
    {
        XINPUT_STATE xi{};
        if (GetJoypadXInputState(DX_INPUT_PAD1, &xi) != 0)
            return;

        auto NormalizeStick = [](SHORT v)
        {
            if (std::abs(v) < STICK_DEAD_ZONE)
                return 0.0f;
            return static_cast<float>(v) / STICK_MAX;
        };

        const float rx = NormalizeStick(xi.ThumbRX);
        const float ry = NormalizeStick(xi.ThumbRY);

        yaw_   += rx * mouseSensitivity_ * 15.0f;
        pitch_ -= ry * mouseSensitivity_ * 15.0f;
    }

    void ThirdPersonCameraBehaviour::UpdateFollowTargetBehaviour() const
    {
        const glm::vec3 targetPos = target_->Transform().GetWorldPos();
        const glm::vec3 lookAtPos = targetPos + lookAtOffsetPos_;

        const glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), yaw_,   glm::vec3(0,1,0));
        const glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), pitch_, glm::vec3(1,0,0));
        const glm::mat4 rot  = rotY * rotX;

        const glm::vec3 offset(0, 0, distance_);
        const glm::vec3 rotatedOffset =
            glm::vec3(rot * glm::vec4(offset, 1.0f));

        follow_->followOffset_ = (lookAtPos + rotatedOffset) - targetPos;
    }

    void ThirdPersonCameraBehaviour::UpdateLookAtTargetBehaviour() const
    {
        const glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), yaw_,   glm::vec3(0,1,0));
        const glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), pitch_, glm::vec3(1,0,0));
        const glm::mat4 rot  = rotY * rotX;

        const auto rotatedOffset = glm::vec3(rot * glm::vec4(lookAtOffsetPos_, 1.0f));

        lookAt_->SetOffsetPos(rotatedOffset);
    }

    void ThirdPersonCameraBehaviour::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("minPitch_", minPitch_);
        LibCore::ImGuiHelper::OnDrawInputField("maxPitch_", maxPitch_);
        LibCore::ImGuiHelper::OnDrawInputField("mouseSensitivity_", mouseSensitivity_);
        LibCore::ImGuiHelper::OnDrawInputField("distance_", distance_);
        LibCore::ImGuiHelper::OnDrawInputField("target_", target_);
        LibCore::ImGuiHelper::OnDrawInputField("follow_", follow_);
        LibCore::ImGuiHelper::OnDrawInputField("lookAt_", lookAt_);
        LibCore::ImGuiHelper::OnDrawInputField("followOffsetPos_", followOffsetPos_);
        LibCore::ImGuiHelper::OnDrawInputField("lookAtOffsetPos_", lookAtOffsetPos_);
        LibCore::ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
    }
}

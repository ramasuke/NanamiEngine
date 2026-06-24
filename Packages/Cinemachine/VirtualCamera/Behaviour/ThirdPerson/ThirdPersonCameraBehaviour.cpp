#include "ThirdPersonCameraBehaviour.h"
#include "../../../Brain/CinemachineCameraBrain.h"
#include "DxLib.h"
#include "../../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../Engine/Module/Physics/RaycastHit/Engine_Physics_RaycastHit.h"
#include "../../../../../Engine/Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"
#include "../../../../../Assets/Scripts/Core/Game/PlayerAvatar/IPlayerAvatar.h"
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

    void ThirdPersonCameraBehaviour::SetEnableLockMousePos(const bool enable)
    {
        isLockMousePos_ = enable;
    }

    void ThirdPersonCameraBehaviour::SetEnableImmediateApply(const bool enable)
    {
        isImmediateApply_ = enable;
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
        if (!isImmediateApply_)
            return;

        if (!cameraBrain_)
            return;

        // ブレインの補完(mix/slerp)を無視し、仮想カメラのTransformを即時適用する。
        // これによりThirdPersonの視点操作がカメラへ遅延なく反映される。
        cameraBrain_->Transform().SetWorldMatrix(Transform().GetWorldMatrix());
    }

    void ThirdPersonCameraBehaviour::UpdateMouseInput()
    {
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);
        
        static int centerX = Core::Application::Configuration::AppConfiguration::GetWindowWidth () / 2;
        static int centerY = Core::Application::Configuration::AppConfiguration::GetWindowHeight() / 2;

        const int dx = mouseX - centerX;
        const int dy = mouseY - centerY;

        if (isLockMousePos_)
        {
            SetMousePoint(centerX, centerY);
        }

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
        const glm::vec3 rotatedOffset = glm::vec3(rot * glm::vec4(offset, 1.0f));

        // 壁などにめり込まないよう、Playerからカメラへrayを飛ばして位置を補正する
        const glm::vec3 adjustedOffset = ResolveCameraCollision(rotatedOffset);

        follow_->followOffset_ = (lookAtPos + adjustedOffset) - targetPos;
    }

    glm::vec3 ThirdPersonCameraBehaviour::ResolveCameraCollision(const glm::vec3& desiredOffset) const
    {
        // Player取得（暫定実装: 本来はネットワーク上の自身が操作するPlayerを取得する）
        const auto& playerAvatars = GameCore::IPlayerAvatar::PlayerAvatars();
        if (playerAvatars.empty())
            return desiredOffset;

        const auto player = playerAvatars.at(0).lock();
        if (!player)
            return desiredOffset;

        const float distance = glm::length(desiredOffset);
        if (distance <= 0.0f)
            return desiredOffset;

        // Playerの注視点を起点に、カメラの理想位置へ向けてrayを飛ばす
        const glm::vec3 origin    = player->PlayerTransform().GetWorldPos() + lookAtOffsetPos_;
        const glm::vec3 direction = desiredOffset / distance;

        Module::Physics::LayerMask mask = Module::Physics::CreateLayerMask();
        Module::Physics::AddLayer(mask, Module::Physics::Layer::Default);

        const Module::Physics::RaycastHit hit = Module::Physics::Raycast(origin, direction, distance, mask);
        if (!hit.Hit())
            return desiredOffset;

        // 障害物の少し手前にカメラを配置する
        const float hitDistance      = glm::length(hit.Position() - origin);
        const float adjustedDistance = std::max(0.0f, hitDistance - collisionBuffer_);

        return direction * adjustedDistance;
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
        ImGuiHelper::OnDrawInputField("minPitch_", minPitch_);
        ImGuiHelper::OnDrawInputField("maxPitch_", maxPitch_);
        ImGuiHelper::OnDrawInputField("mouseSensitivity_", mouseSensitivity_);
        ImGuiHelper::OnDrawInputField("distance_", distance_);
        ImGuiHelper::OnDrawInputField("collisionBuffer_", collisionBuffer_);
        ImGuiHelper::OnDrawInputField("target_", target_);
        ImGuiHelper::OnDrawInputField("follow_", follow_);
        ImGuiHelper::OnDrawInputField("lookAt_", lookAt_);
        ImGuiHelper::OnDrawInputField("followOffsetPos_", followOffsetPos_);
        ImGuiHelper::OnDrawInputField("lookAtOffsetPos_", lookAtOffsetPos_);
        ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
        ImGuiHelper::OnDrawInputField("isImmediateApply_", isImmediateApply_);
    }
}

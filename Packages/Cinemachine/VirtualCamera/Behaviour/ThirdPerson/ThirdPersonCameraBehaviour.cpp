#include "ThirdPersonCameraBehaviour.h"
#include "../../../Brain/CinemachineCameraBrain.h"
#include "DxLib.h"
#include "../../../../../Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../Engine/Module/Physics/RaycastHit/Engine_Physics_RaycastHit.h"
#include "../../../../../Engine/Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"
#include "../IVirtualCameraTarget.h"
#include "gtx/rotate_vector.hpp"
#include "../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    constexpr float STICK_MAX       = 32767.0f;
    constexpr int   STICK_DEAD_ZONE = 8000;
    // NOTE: カメラが止まったら数フレームで固定が外れた扱いにする
    constexpr int   MOUSE_PIN_HOLD_MS = 100;
}

namespace NanamiEngine::CineMachine::Behaviour
{
    // NOTE: 初回の差分が大きくても固定していない扱いになるよう、十分昔にしておく
    int ThirdPersonCameraBehaviour::lastMousePinnedMs_ = -MOUSE_PIN_HOLD_MS * 100;

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

    bool ThirdPersonCameraBehaviour::IsMousePinned()
    {
        return GetNowCount() - lastMousePinnedMs_ < MOUSE_PIN_HOLD_MS;
    }

    void ThirdPersonCameraBehaviour::OnAwake()
    {
        follow_ = RequireComponent<VirtualCameraFollowBehaviour>();
        lookAt_ = RequireComponent<VirtualCameraLookAtBehaviour>();
    }

    void ThirdPersonCameraBehaviour::OnCameraUpdate()
    {
        if (!target_)
            return;

        // NOTE: Brain は有効/無効を見ずに呼ぶので、ここで止める。Follow/LookAt は最後の offset のまま追従する
        if (!IsEnable())
        {
            isMouseDeltaStale_ = true;
            return;
        }

        UpdateMouseInput();
        UpdateGamepadInput();

        pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);

        UpdateFollowTargetBehaviour();
        UpdateLookAtTargetBehaviour();
    }

    void ThirdPersonCameraBehaviour::UpdateMouseInput()
    {
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);
        
        // NOTE: ウィンドウサイズが変わっても追従するよう毎回求める
        const int centerX = Core::Application::Configuration::AppConfiguration::GetWindowWidth () / 2;
        const int centerY = Core::Application::Configuration::AppConfiguration::GetWindowHeight() / 2;

        const int dx = mouseX - centerX;
        const int dy = mouseY - centerY;

        if (isLockMousePos_)
        {
            SetMousePoint(centerX, centerY);
            lastMousePinnedMs_ = GetNowCount();
        }

        // NOTE: 無効の間はカーソルが自由に動くので、戻った最初の差分は捨てる(カメラが跳ねる)
        if (isMouseDeltaStale_)
        {
            isMouseDeltaStale_ = false;
            return;
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
        const glm::vec3 targetPos = IVirtualCameraTarget::PositionOf(*target_.get());
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
        const float distance = glm::length(desiredOffset);
        if (distance <= 0.0f)
            return desiredOffset;

        // 追従対象の注視点を起点に、カメラの理想位置へ向けてrayを飛ばす
        const glm::vec3 origin    = IVirtualCameraTarget::PositionOf(*target_.get()) + lookAtOffsetPos_;
        const glm::vec3 direction = desiredOffset / distance;

        Module::Physics::LayerMask mask = Module::Physics::CreateLayerMask();
        Module::Physics::AddLayer(mask, Module::Physics::Layer::Default);

        // 太さ0のRayだと横壁や地面すれすれでNear平面がめり込むため、半径を持った球で位置を決める。
        // これでカメラ周囲に最低collisionRadius_の空きが保証され、Brainの動的Nearが極端に小さくならない
        Module::Physics::RaycastHit hit = Module::Physics::SphereCast(origin, collisionRadius_, direction, distance, mask);
        if (hit.Hit() && hit.Distance() <= 0.0f)
        {
            // 始点(注視点)の時点で球が既に壁に重なっている場合、球では位置が決まらないためRayにフォールバックする
            hit = Module::Physics::Raycast(origin, direction, distance, mask);
        }
        if (!hit.Hit())
            return desiredOffset;

        // 障害物の少し手前にカメラを配置する
        const float adjustedDistance = std::max(0.0f, hit.Distance() - collisionBuffer_);

        return direction * adjustedDistance;
    }

    void ThirdPersonCameraBehaviour::UpdateLookAtTargetBehaviour() const
    {
        // pitchでも回すと見上げた時に注視点がカメラ側へ回り込み、プレイヤーが画面外へ出るためyawのみ
        const glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), yaw_, glm::vec3(0,1,0));

        const auto rotatedOffset = glm::vec3(rotY * glm::vec4(lookAtOffsetPos_, 1.0f));

        lookAt_->SetOffsetPos(rotatedOffset);
    }

    void ThirdPersonCameraBehaviour::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("minPitch_", minPitch_);
        ImGuiHelper::OnDrawInputField("maxPitch_", maxPitch_);
        ImGuiHelper::OnDrawInputField("mouseSensitivity_", mouseSensitivity_);
        ImGuiHelper::OnDrawInputField("distance_", distance_);
        ImGuiHelper::OnDrawInputField("collisionBuffer_", collisionBuffer_);
        ImGuiHelper::OnDrawInputField("collisionRadius_", collisionRadius_);
        ImGuiHelper::OnDrawInputField("target_", target_);
        ImGuiHelper::OnDrawInputField("follow_", follow_);
        ImGuiHelper::OnDrawInputField("lookAt_", lookAt_);
        ImGuiHelper::OnDrawInputField("followOffsetPos_", followOffsetPos_);
        ImGuiHelper::OnDrawInputField("lookAtOffsetPos_", lookAtOffsetPos_);
        ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
        ImGuiHelper::OnDrawInputField("isImmediateApply_", isImmediateApply_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(CineMachine::Behaviour::ThirdPersonCameraBehaviour);
NANAMI_REGISTER_POLYMORPHIC_RELATION(LifeCycleCallback::IAwakable, CineMachine::Behaviour::ThirdPersonCameraBehaviour);
NANAMI_REGISTER_POLYMORPHIC_RELATION(CineMachine::IVirtualCameraBehaviour, CineMachine::Behaviour::ThirdPersonCameraBehaviour);
#pragma endregion

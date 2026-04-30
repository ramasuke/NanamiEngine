#include "Editor3DCamera.h"
#include <DxLib.h>

#include "../../../Configuration/ApplicationConfiguration.h"
#include "ext/quaternion_trigonometric.hpp"
#include "gtx/euler_angles.hpp"

namespace NanamiEngine::Module::Component
{
    Editor3DCamera::Editor3DCamera()
        : previewMousePos_()
    {
        cameraRotation_ = {glm::quat(1, 0, 0, 0)};
        cameraPosition_ = {0.0f, 0.0f, 5.0f};
        matrix_ = {1.0f};
    
        SetUseZBuffer3D(true);
        SetWriteZBuffer3D(true);
    }
    
    void Editor3DCamera::OnUpdate()
    {
        OnUpdateCameraRotation();
        OnUpdateCameraPosition();
    
        matrix_ = glm::translate(glm::mat4(1.0f), cameraPosition_) * glm::mat4_cast(cameraRotation_);
    
        SetCameraNearFar(5.0f, 2300.0f);
    
        const auto worldPos = glm::vec3(matrix_[3]);
        const auto forward  = cameraRotation_ * glm::vec3(0, 0, 1);
        const auto target   = worldPos + forward;
    
        constexpr auto sampleCameraFov = 90.0f;
        SetupCamera_Perspective(sampleCameraFov  * DX_PI_F / 180.0f );
        SetCameraPositionAndTarget_UpVecY(
            {worldPos.x, worldPos.y, worldPos.z},
            {target  .x, target  .y, target  .z}
        );
        
        GetMousePoint(&previewMousePos_.x, &previewMousePos_.y);
    }
    
    glm::mat4 Editor3DCamera::GetViewMatrix() const
    {
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), cameraPosition_);
        const glm::mat4 rotation    = glm::toMat4(cameraRotation_);
        const glm::mat4 world = translation * rotation;
    
        return glm::inverse(world);
    }
    
    
    glm::mat4 Editor3DCamera::GetProjectionMatrix() const
    {
        constexpr float aspect = static_cast<float>(Core::Application::Configuration::WINDOW_WIDTH_SIZE) / static_cast<float>(Core::Application::Configuration::WINDOW_HEIGHT_SIZE);
        return glm::perspectiveLH_ZO(fov_, aspect, near_, far_);
    }
    
    
    void Editor3DCamera::OnUpdateCameraRotation()
    {
        if (GetMouseInput() != MOUSE_INPUT_RIGHT)
            return;
    
        glm::ivec2 currentMousePos{};
        GetMousePoint(&currentMousePos.x, &currentMousePos.y);
    
        const auto mouseDelta = glm::vec2(currentMousePos - previewMousePos_);
        previewMousePos_ = currentMousePos;
    
        const float yawAmount   = mouseDelta.x * sensitivity_;
        const float pitchAmount = mouseDelta.y * sensitivity_;
    
        const glm::quat currentRot = cameraRotation_;
        const glm::vec3 forward    = currentRot * glm::vec3(0, 0, 1);
        const float currentPitch   = std::asin(glm::clamp(forward.y, -1.0f, 1.0f));
    
        constexpr float maxPitch = glm::radians(89.0f);
        const float newPitch = glm::clamp(currentPitch + pitchAmount, -maxPitch, maxPitch);
        const float clampedPitchAmount = newPitch - currentPitch;
    
        constexpr auto worldUp = glm::vec3(0, 1, 0);
        const glm::vec3 localRight = currentRot * glm::vec3(1, 0, 0);
    
        cameraRotation_ = glm::normalize(
            glm::angleAxis(clampedPitchAmount, localRight) *
            glm::angleAxis(yawAmount, worldUp) *
            currentRot
        );
    }
    
    void Editor3DCamera::OnUpdateCameraPosition()
    {
        if (GetMouseInput() != MOUSE_INPUT_RIGHT)
            return;
    
        glm::vec3 move{0.0f};
    
        glm::vec3 forward = cameraRotation_ * glm::vec3(0, 0, 1);
        glm::vec3 right   = cameraRotation_ * glm::vec3(1, 0, 0);
    
        forward = glm::normalize(forward);
        right   = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
    
        if (CheckHitKey(KEY_INPUT_W     )) move   += forward * moveSpeed_;
        if (CheckHitKey(KEY_INPUT_S     )) move   -= forward * moveSpeed_;
        if (CheckHitKey(KEY_INPUT_D     )) move   += right   * moveSpeed_;
        if (CheckHitKey(KEY_INPUT_A     )) move   -= right   * moveSpeed_;
        if (CheckHitKey(KEY_INPUT_SPACE )) move.y += moveSpeed_;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) move   *= moveRunSpeedCoefficient_;
    
        cameraPosition_ += move;
    }
}

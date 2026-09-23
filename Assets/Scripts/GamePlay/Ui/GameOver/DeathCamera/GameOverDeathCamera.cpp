#include "GameOverDeathCamera.h"

#include <algorithm>
#include <cmath>

#include "geometric.hpp"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float GAME_OVER_CAMERA_DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
    }

    void GameOverDeathCamera::Begin(const std::shared_ptr<GameObject::IGameObject>& target)
    {
        const auto* brain = CineMachine::CinemachineCameraBrain::Instance();
        const auto camera = RequireComponent<CineMachine::CineMachineVirtualCamera>();
        const auto follow = Components().Catch<CineMachine::Behaviour::VirtualCameraFollowBehaviour>().lock();
        const auto lookAt = Components().Catch<CineMachine::Behaviour::VirtualCameraLookAtBehaviour>().lock();
        if (!brain || !target || !follow || !lookAt)
            return;

        target_ = target;
        follow_ = follow;
        lookAt_ = lookAt;

        // 今映っている画から始める。Follow/LookAt が動く前のフレームで Brain が原点へ補間しないよう、姿勢も写しておく
        const glm::vec3 cameraPos = brain->Transform().GetWorldPos();
        const glm::vec3 forward = brain->Transform().GetWorldRot() * glm::vec3(0.0f, 0.0f, 1.0f);
        Transform().SetWorldMatrix(brain->Transform().GetWorldMatrix());

        const glm::vec3 targetPos = target->Transform().GetWorldPos();
        const glm::vec3 offset = cameraPos - targetPos;
        startDistance_ = glm::length(offset);
        if (startDistance_ > 0.001f)
        {
            startYaw_rad_ = std::atan2(offset.x, offset.z);
            startPitch_rad_ = std::asin(std::clamp(offset.y / startDistance_, -1.0f, 1.0f));
        }
        else
        {
            startDistance_ = endDistance_;
            startYaw_rad_ = 0.0f;
            startPitch_rad_ = endPitchDeg_ * GAME_OVER_CAMERA_DEG_TO_RAD;
        }

        // 視線の上で、倒れた位置と同じ奥行きの点を最初の注視点にする
        const glm::vec3 lookPoint = cameraPos + forward * glm::dot(targetPos - cameraPos, forward);
        startLookAtOffset_ = lookPoint - targetPos;

        follow->SetTarget(target);
        lookAt->SetTarget(target);
        shotTween_.Play(tweeny::from(0.0f).to(1.0f)
            .during(LibCore::Tween::Ms(shotSecs_))
            .via(LibCore::Tween::Ease(LibCore::EaseType::OutQuad)));
        isPlaying_ = true;
        ApplyShot(0.0f);

        camera->SetPriority(priority_);
    }

    void GameOverDeathCamera::OnUpdate()
    {
        if (!isPlaying_)
            return;

        if (target_.expired())
        {
            isPlaying_ = false;
            if (const auto camera = Components().Catch<CineMachine::CineMachineVirtualCamera>().lock())
                camera->OnDisable();
            return;
        }

        shotTween_.Tick(Time::DeltaTime());
        ApplyShot(shotTween_.Value());
    }

    void GameOverDeathCamera::ApplyShot(const float rate) const
    {
        const auto follow = follow_.lock();
        const auto lookAt = lookAt_.lock();
        if (!follow || !lookAt)
            return;

        // 水平角0が +Z 側。GrassLandArrivalMovie と同じ極座標で置く
        const float yaw = startYaw_rad_ + orbitDeg_ * GAME_OVER_CAMERA_DEG_TO_RAD * rate;
        const float pitch = std::lerp(startPitch_rad_, endPitchDeg_ * GAME_OVER_CAMERA_DEG_TO_RAD, rate);
        const float distance = std::lerp(startDistance_, endDistance_, rate);

        follow->followOffset_ = glm::vec3(
            distance * std::cos(pitch) * std::sin(yaw),
            distance * std::sin(pitch),
            distance * std::cos(pitch) * std::cos(yaw));
        lookAt->SetOffsetPos(startLookAtOffset_ + (endLookAtOffset_ - startLookAtOffset_) * rate);
    }

    void GameOverDeathCamera::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("priority_", priority_);
        ImGuiHelper::OnDrawInputField("shotSecs_", shotSecs_);
        ImGuiHelper::OnDrawInputField("orbitDeg_", orbitDeg_);
        ImGuiHelper::OnDrawInputField("endPitchDeg_", endPitchDeg_);
        ImGuiHelper::OnDrawInputField("endDistance_", endDistance_);
        ImGuiHelper::OnDrawInputField("endLookAtOffset_", endLookAtOffset_);
        ImGui::Text("playing: %d  progress: %.2f", isPlaying_ ? 1 : 0, shotTween_.Progress());
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::GameOverDeathCamera);
#pragma endregion

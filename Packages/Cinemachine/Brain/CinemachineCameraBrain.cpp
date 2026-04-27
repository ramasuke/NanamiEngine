#include "CinemachineCameraBrain.h"

#include "../../../Engine/Core/Application/Time/Time.h"
#include "../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../Engine/Module/GameObject/Transform/Transform.h"

CineMachine::CinemachineCameraBrain* CineMachine::CinemachineCameraBrain::cameraBrain_ = nullptr;

void CineMachine::CinemachineCameraBrain::OnAwake()
{
    
}

void CineMachine::CinemachineCameraBrain::OnStart()
{
    
}

void CineMachine::CinemachineCameraBrain::OnUpdate()
{
    if (!currentVirtualCamera_)
        return;

    const glm::vec3 currentPos = Transform().GetWorldPos();
    const glm::quat currentRot = Transform().GetWorldRot();

    const glm::vec3 targetPos = currentVirtualCamera_->Transform().GetWorldPos();
    const glm::quat targetRot = currentVirtualCamera_->Transform().GetWorldRot();

    const float dt = Time::DeltaTime();

    const glm::vec3 newPos = glm::mix(currentPos, targetPos, 1.0f - std::exp(-positionLerpSpeed_secs_ * dt));
    const glm::quat newRot = glm::slerp(currentRot, targetRot, 1.0f - std::exp(-rotationSlerpSpeed_secs_ * dt));

    Transform().SetWorldPos(newPos);
    Transform().SetWorldRot(newRot);

    const glm::vec3 forward = newRot * glm::vec3(0, 0, 1);

    SetupCamera_Perspective(fov_ * DX_PI_F / 180.0f);
    SetCameraNearFar(cameraNear_, cameraFar_);
    SetCameraPositionAndTarget_UpVecY(
        {newPos.x, newPos.y, newPos.z},
        {newPos.x + forward.x, newPos.y + forward.y, newPos.z + forward.z}
    );
    currentVirtualCamera_->MainCameraCallback();
}

void CineMachine::CinemachineCameraBrain::OnDebugRender()
{
    if (currentVirtualCamera_)
    {
        if (!Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
        {
            Transform().SetWorldPos(currentVirtualCamera_->Transform().GetWorldPos());
            Transform().SetWorldRot(currentVirtualCamera_->Transform().GetWorldRot());
        }
        else
        {
            Transform().SetWorldPos(Transform().GetWorldPos());
            Transform().SetWorldRot(Transform().GetWorldRot());
        }
    }
    OnDebugCameraFovRender();
}

void CineMachine::CinemachineCameraBrain::OnDebugCameraFovRender() const
{
    const glm::vec3 eye = Transform().GetWorldPos();
    const glm::quat rot = Transform().GetWorldRot();

    const glm::vec3 forward = rot * glm::vec3(0, 0, 1);
    const glm::vec3 up      = rot * glm::vec3(0, 1, 0);
    const glm::vec3 right   = rot * glm::vec3(1, 0, 0);

    int screenWidth, screenHeight;
    GetScreenState(&screenWidth, &screenHeight, nullptr);

    constexpr float fovRad      = SAMPLE_CAMERA_FOV * DX_PI_F / 180.0f;
    constexpr float debugFar    = 80.0f;

    const float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    const float halfHeight  = tanf(fovRad * 0.5f) * debugFar;
    const float halfWidth   = halfHeight * aspectRatio;
    const glm::vec3 farCenter = eye + forward * debugFar;

    const glm::vec3 p1 = farCenter + up * halfHeight + right * halfWidth;
    const glm::vec3 p2 = farCenter + up * halfHeight - right * halfWidth;
    const glm::vec3 p3 = farCenter - up * halfHeight - right * halfWidth;
    const glm::vec3 p4 = farCenter - up * halfHeight + right * halfWidth;

    const int color = GetColor(255, 255, 0);

    DrawTriangle3D({eye.x, eye.y, eye.z}, {p1.x, p1.y, p1.z}, {p2.x, p2.y, p2.z}, color, false);
    DrawTriangle3D({eye.x, eye.y, eye.z}, {p2.x, p2.y, p2.z}, {p3.x, p3.y, p3.z}, color, false);
    DrawTriangle3D({eye.x, eye.y, eye.z}, {p3.x, p3.y, p3.z}, {p4.x, p4.y, p4.z}, color, false);
    DrawTriangle3D({eye.x, eye.y, eye.z}, {p4.x, p4.y, p4.z}, {p1.x, p1.y, p1.z}, color, false);
}

void CineMachine::CinemachineCameraBrain::OnDrawGui()
{
    if (currentVirtualCamera_)
    {
        ImGui::Text(("currentVirtualCameraPriority: " + std::to_string(currentVirtualCamera_->Priority().Value())).c_str());
    }
    else
    {
        ImGui::Text("currentVirtualCameraPriority: none");
    }
    ImGuiHelper::OnDrawInputField("currentVirtualCamera_", currentVirtualCamera_);

    ImGuiHelper::OnDrawInputField("virtualCameras_", virtualCameras_, [this]
    {
        if (ImGui::Button("Add"))
        {
            virtualCameras_.emplace_back();
        }
    });
    
    ImGuiHelper::OnDrawInputField("positionLerpSpeed_secs_"  , positionLerpSpeed_secs_   );
    ImGuiHelper::OnDrawInputField("rotationSlerpSpeed_secs_" , rotationSlerpSpeed_secs_  );
    ImGuiHelper::OnDrawInputField("fov_"                     , fov_                      );
    ImGuiHelper::OnDrawInputField("cameraNear_"              , cameraNear_               );
    ImGuiHelper::OnDrawInputField("cameraFar_"               , cameraFar_                );
}

void CineMachine::CinemachineCameraBrain::ApplyVirtualCameraMatrix() const
{
    Transform().SetWorldMatrix(currentVirtualCamera_->Transform().GetWorldMatrix());
}

void CineMachine::CinemachineCameraBrain::ApplyVirtualCameraMatrix(
    const CineMachineVirtualCamera& virtualCamera) const
{
    Transform().SetWorldMatrix(virtualCamera.Transform().GetWorldMatrix());
}

void CineMachine::CinemachineCameraBrain::SubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera)
{
    cameraBrain_->virtualCameras_.emplace_back(virtualCamera);
    virtualCamera.lock()->Priority().Subscribe(
            [](int)
            {
                const auto highestPriorityVirtualCamera
                    = *std::ranges::max_element(cameraBrain_->virtualCameras_,
                            [](auto& a, auto& b)
                            {
                                return a->Priority().Value() < b->Priority().Value();
                            });

                cameraBrain_->currentVirtualCamera_ = highestPriorityVirtualCamera;
            });
}

void CineMachine::CinemachineCameraBrain::UnSubscribeVirtualCamera(
    const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera)
{
    if (cameraBrain_ == nullptr)
        return;

    // assert(!cameraBrain_->virtualCameras_.empty() && "No virtual cameras registered!");

    //TODO: 明らかにバグです、修正必須。
    if (cameraBrain_->virtualCameras_.empty())
        return;
    
    std::erase_if(cameraBrain_->virtualCameras_,
        [&](const Core::Object::Field<CineMachineVirtualCamera>& camera)
        {
            return camera.get() == virtualCamera.lock();
        });
    
    cameraBrain_->currentVirtualCamera_ =
        *std::ranges::max_element(
            cameraBrain_->virtualCameras_,
            [](auto& a, auto& b)
            {
                return a->Priority().Value()
                     < b->Priority().Value();
            });
}

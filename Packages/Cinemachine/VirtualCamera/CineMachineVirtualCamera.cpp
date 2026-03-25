#include "CineMachineVirtualCamera.h"

#include "../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../Brain/CinemachineCameraBrain.h"
#include "Behaviour/Follow/VirtualCameraFollowBehaviour.h"
#include "Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"
#include "Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"

void CineMachine::CineMachineVirtualCamera::SetPriority(const int priority)
{
    priority_.OnNext(priority);
}

void CineMachine::CineMachineVirtualCamera::MainCameraCallback() const
{
    for (const auto& cameraBehaviour : cameraBehaviours_)
    {
        cameraBehaviour.lock()->MainCameraCallback();
    }
}

void CineMachine::CineMachineVirtualCamera::OnAwake()
{
    cameraBehaviours_ = Components().Catches<IVirtualCameraBehaviour>();
    CinemachineCameraBrain::SubscribeVirtualCamera(Components().Catch<CineMachineVirtualCamera>());
}

void CineMachine::CineMachineVirtualCamera::OnDestroy()
{
    CinemachineCameraBrain::UnSubscribeVirtualCamera(Components().Catch<CineMachineVirtualCamera>());
}

void CineMachine::CineMachineVirtualCamera::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("priority_", priority_);
    
    if (ImGui::Button("AddCameraBehaviour"))
    {
        ImGui::OpenPopup("AddCameraBehaviourPopup");
    }

    if (ImGui::BeginPopup("AddCameraBehaviourPopup"))
    {
        ImGui::Text("Virtual Camera Behaviour");
        ImGui::Separator();

        if (ImGui::Button("Add Follow"     )) Components().Add<Behaviour::VirtualCameraFollowBehaviour>();
        if (ImGui::Button("Add LookAt"     )) Components().Add<Behaviour::VirtualCameraLookAtBehaviour>();
        if (ImGui::Button("Add ThirdPerson")) Components().Add<Behaviour::ThirdPersonCameraBehaviour  >();
        ImGui::EndPopup();
    }
}

void CineMachine::CineMachineVirtualCamera::OnDebugRender()
{
    const glm::vec3 eye     = Transform().GetWorldPos();
    const glm::vec3 forward = Transform().GetWorldRot() * glm::vec3(0, 0, 1);
    constexpr auto worldUp = glm::vec3(0, 1, 0);

    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up    = glm::normalize(glm::cross(right, forward));

    int screenWidth, screenHeight;
    GetScreenState(&screenWidth, &screenHeight, nullptr);
    const float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

    constexpr float fovRad = SAMPLE_CAMERA_FOV * DX_PI_F / 180.0f;
    constexpr float debugFar = 80.0f;
    const float halfHeight = tanf(fovRad * 0.5f) * debugFar;
    const float halfWidth = halfHeight * aspectRatio;
    const glm::vec3 farCenter = eye + forward * debugFar;
    
    const glm::vec3 p1 = farCenter + up * halfHeight + right * halfWidth;
    const glm::vec3 p2 = farCenter + up * halfHeight - right * halfWidth;
    const glm::vec3 p3 = farCenter - up * halfHeight - right * halfWidth;
    const glm::vec3 p4 = farCenter - up * halfHeight + right * halfWidth;

    const int color = GetColor(200, 200, 200);

    //DrawTriangle3D({eye.x, eye.y, eye.z}, {p1.x, p1.y, p1.z}, {p2.x, p2.y, p2.z}, color, false);
    //DrawTriangle3D({eye.x, eye.y, eye.z}, {p2.x, p2.y, p2.z}, {p3.x, p3.y, p3.z}, color, false);
    //DrawTriangle3D({eye.x, eye.y, eye.z}, {p3.x, p3.y, p3.z}, {p4.x, p4.y, p4.z}, color, false);
    //DrawTriangle3D({eye.x, eye.y, eye.z}, {p4.x, p4.y, p4.z}, {p1.x, p1.y, p1.z}, color, false);
}

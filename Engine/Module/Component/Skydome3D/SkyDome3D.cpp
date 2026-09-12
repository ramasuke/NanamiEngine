#include "SkyDome3D.h"

#include "../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../../GameObject/Transform/Transform.h"

namespace
{
    MATRIX BuildSkyDomeMatrix(const VECTOR& worldPos, const glm::quat& worldRot, const glm::vec3& worldScale)
    {
        const glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(worldPos.x, worldPos.y, worldPos.z))
                            * glm::toMat4(worldRot)
                            * glm::scale(glm::mat4(1.0f), worldScale);

        MATRIX dxMat;
        dxMat.m[0][0] = mat[0][0]; dxMat.m[0][1] = mat[0][1]; dxMat.m[0][2] = mat[0][2]; dxMat.m[0][3] = mat[0][3];
        dxMat.m[1][0] = mat[1][0]; dxMat.m[1][1] = mat[1][1]; dxMat.m[1][2] = mat[1][2]; dxMat.m[1][3] = mat[1][3];
        dxMat.m[2][0] = mat[2][0]; dxMat.m[2][1] = mat[2][1]; dxMat.m[2][2] = mat[2][2]; dxMat.m[2][3] = mat[2][3];
        dxMat.m[3][0] = mat[3][0]; dxMat.m[3][1] = mat[3][1]; dxMat.m[3][2] = mat[3][2]; dxMat.m[3][3] = mat[3][3];
        return dxMat;
    }
}

void Component::SkyDome3D::InitRenderer()
{
    if (skyDomeModel_)
    {
        skyDomeModelDxLibHandle_ = skyDomeModel_->LoadDxLibHandle();
    }
}

void Component::SkyDome3D::OnUpdate()
{
    if (mainCamera_)
    {
        MV1SetMatrix(skyDomeModelDxLibHandle_, BuildSkyDomeMatrix(
            mainCamera_->Transform().GetDxWorldPos(), Transform().GetWorldRot(), Transform().GetWorldScale()));
    }
}

void Component::SkyDome3D::OnRender()
{
    MV1DrawModel(skyDomeModelDxLibHandle_);
}

void Component::SkyDome3D::OnDebugRender()
{
    if (!Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
    {
        MV1SetMatrix(skyDomeModelDxLibHandle_, BuildSkyDomeMatrix(
            Core::Application::ApplicationBase::GameWindow()->GetCameraDxPosition(), Transform().GetWorldRot(), Transform().GetWorldScale()));
    }
}

void Component::SkyDome3D::OnDestroy()
{
    MV1DeleteModel(skyDomeModelDxLibHandle_);
}

void Component::SkyDome3D::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("skyDomeModel_", skyDomeModel_);
    ImGuiHelper::OnDrawInputField("skyDomeModelDxLibHandle_", skyDomeModelDxLibHandle_);
    ImGuiHelper::OnDrawInputField("mainCamera_", mainCamera_);
}

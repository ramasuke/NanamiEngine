#include "SkyDome3D.h"

#include "../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../../GameObject/Transform/Transform.h"

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
        MV1SetPosition(skyDomeModelDxLibHandle_, mainCamera_->Transform().GetDxWorldPos());
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
        MV1SetPosition(skyDomeModelDxLibHandle_, Core::Application::ApplicationBase::GameWindow()->GetCameraDxPosition());
    }
}

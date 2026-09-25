#include "Camera.h"

#include "DxLib.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace NanamiEngine::Platform::Render::Camera
{
    glm::vec3 Position()
    {
        return LibCore::Dxlib::FromDxVector(GetCameraPosition());
    }

    float Fov()
    {
        return GetCameraFov();
    }

    glm::vec3 UpVector()
    {
        return LibCore::Dxlib::FromDxVector(GetCameraUpVector());
    }

    glm::vec3 WorldToScreen(const glm::vec3& worldPosition)
    {
        return LibCore::Dxlib::FromDxVector(ConvWorldPosToScreenPos(LibCore::Dxlib::ToDxVector(worldPosition)));
    }

    glm::vec3 ScreenToWorld(const glm::vec3& screenPosition)
    {
        return LibCore::Dxlib::FromDxVector(ConvScreenPosToWorldPos(LibCore::Dxlib::ToDxVector(screenPosition)));
    }

    bool IsBoxOutsideView(const glm::vec3& boxMin, const glm::vec3& boxMax)
    {
        return CheckCameraViewClip_Box(LibCore::Dxlib::ToDxVector(boxMin), LibCore::Dxlib::ToDxVector(boxMax)) == TRUE;
    }
}

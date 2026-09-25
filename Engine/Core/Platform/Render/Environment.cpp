#include "Environment.h"

#include "DxLib.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace NanamiEngine::Platform::Render::Environment
{
    void SetFogEnabled(const bool enabled)
    {
        SetFogEnable(enabled ? TRUE : FALSE);
    }

    void SetFogColor(const Color32& color)
    {
        DxLib::SetFogColor(color.R(), color.G(), color.B());
    }

    void SetFogStartEnd(const float start, const float end)
    {
        DxLib::SetFogStartEnd(start, end);
    }

    void SetLightDiffuseColor(const glm::vec3& rgb01)
    {
        SetLightDifColor(GetColorF(rgb01.r, rgb01.g, rgb01.b, 1.0f));
    }

    glm::vec3 GetLightDiffuseColor()
    {
        const COLOR_F color = GetLightDifColor();
        return { color.r, color.g, color.b };
    }

    glm::vec3 GetLightDirection()
    {
        return LibCore::Dxlib::FromDxVector(DxLib::GetLightDirection());
    }
}

#include "Billboard.h"

#include "DxLib.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace NanamiEngine::Platform::Render::Billboard
{
    void Draw(const glm::vec3& center, const float size, const float angle, const int graphHandle)
    {
        DrawBillboard3D(LibCore::Dxlib::ToDxVector(center), 0.5f, 0.5f, size, angle, graphHandle, TRUE);
    }
}

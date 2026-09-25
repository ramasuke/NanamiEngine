#include "Model.h"

#include "DxLib.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"
#include "../../../../Libs/LibCore/DxLib/ShiftJis.h"

namespace NanamiEngine::Platform::Render::Model
{
    int SearchFrame(const int modelHandle, const std::string& utf8FrameName)
    {
        return MV1SearchFrame(modelHandle, LibCore::Dxlib::Utf8ToShiftJis(utf8FrameName).c_str());
    }

    glm::mat4 GetMatrix(const int modelHandle)
    {
        return LibCore::Dxlib::FromDxMatrix(MV1GetMatrix(modelHandle));
    }

    glm::mat4 GetFrameLocalWorldMatrix(const int modelHandle, const int frameIndex)
    {
        return LibCore::Dxlib::FromDxMatrix(MV1GetFrameLocalWorldMatrix(modelHandle, frameIndex));
    }
}

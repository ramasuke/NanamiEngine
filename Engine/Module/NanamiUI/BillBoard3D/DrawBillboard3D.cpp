#include "DrawBillboard3D.h"

#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::NanamiUi
{
    void Billboard3D::InitRenderer()
    {
        
    }

    void Billboard3D::OnUserInterfaceRender()
    {
        if (!IsEnable() || !spriteFile_)
            return;

        DrawBillboard3D(
            Transform().GetDxWorldPos(),
            0.5f,
            0.5f,
            Transform().GetWorldScale().x,
            Transform().GetWorldEulerAngle().z,
            spriteFile_->GetDxLibHandle(),
            true
        );
    }

    void Billboard3D::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("billboardSprite_", spriteFile_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
    }
}

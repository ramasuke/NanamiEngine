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
        if (!IsEnable())
        return;

        if (!IsEnable() || !sprite_)
            return;

        DrawBillboard3D(
            Transform().GetDxWorldPos(),
            0.0f,
            0.0f,
            Transform().GetWorldScale().x,
            Transform().GetWorldEulerAngle().z,
            sprite_->GetDxLibHandle(),
            true
        );
    }

    void Billboard3D::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("billboardSprite_", sprite_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
    }
}

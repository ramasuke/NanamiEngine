#include "DrawBillboard3D.h"

#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Module::NanamiUi
{
    void Billboard3D::InitRenderer()
    {
        handle_ = spriteFile_->LoadGraph();
    }

    void Billboard3D::OnUserInterfaceRender()
    {
        if (!IsEnable() || !spriteFile_)
            return;

        auto a = DrawBillboard3D(
            Transform().GetDxWorldPos(),
            0.5f,
            0.5f,
            Transform().GetWorldScale().x,
            angle_,
            handle_,
            true
        );
        
        assert(a != -1, "error");
    }

    void Billboard3D::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("billboardSprite_", spriteFile_);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        ImGuiHelper::OnDrawInputField("angle_", angle_);
    }
}

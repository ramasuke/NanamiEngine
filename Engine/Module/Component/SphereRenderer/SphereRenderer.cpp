#include "SphereRenderer.h"

#include <DxLib.h>

#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::Component
{
    void SphereRenderer::OnRender()
    {
        DrawSphere3D(
            Transform().GetDxWorldPos(),
            radius_,
            divNum_,
            color_    .ToDxColor(),
            edgeColor_.ToDxColor(),
            fill_ ? TRUE : FALSE
        );
    }

    void SphereRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("radius_"   , radius_   );
        ImGuiHelper::OnDrawInputField("divNum_"   , divNum_   );
        ImGuiHelper::OnDrawInputField("color_"    , color_    );
        ImGuiHelper::OnDrawInputField("edgeColor_", edgeColor_);
        ImGuiHelper::OnDrawInputField("fill_"     , fill_     );
    }
}

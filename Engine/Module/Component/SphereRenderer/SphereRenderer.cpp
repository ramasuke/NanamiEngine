#include "SphereRenderer.h"

#include <DxLib.h>

#include "../../GameObject/Transform/Transform.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"
#include "../../../../Libs/LibCore/DxLib/DxMath.h"

namespace NanamiEngine::Module::Component
{
    void SphereRenderer::OnRender()
    {
        if (!IsEnable())
            return;

        DrawSphere3D(
            LibCore::Dxlib::ToDxVector(Transform().GetWorldPos()),
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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::SphereRenderer);
#pragma endregion

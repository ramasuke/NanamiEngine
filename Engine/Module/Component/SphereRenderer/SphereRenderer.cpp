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
            color_.ToDxColor(),
            edgeColor_.ToDxColor(),
            fill_ ? TRUE : FALSE
        );
    }
}

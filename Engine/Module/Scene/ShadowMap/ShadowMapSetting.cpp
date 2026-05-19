#include "ShadowMapSetting.h"

#include "vec3.hpp"

namespace NanamiEngine::Scene
{
    glm::vec3 ShadowMapSetting::renderAreaSize_ = glm::vec3{100.0f, 100.0f, 100.0f};
    glm::vec3 ShadowMapSetting::renderAreaPos_  = glm::vec3{0.0f, 0.0f, 0.0f};
    
    void ShadowMapSetting::SetRenderAreaSize(
        const glm::vec3 renderAreaSize)
    {
        renderAreaSize_ = renderAreaSize;
    }

    void ShadowMapSetting::SetRenderAreaPos(
        const glm::vec3 position)
    {
        renderAreaPos_ = position;
    }
}

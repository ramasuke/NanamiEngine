#pragma once
#include "../glm/vec3.hpp"

namespace NanamiEngine::Scene
{
    class ShadowMapSetting final
    {
    public:
        static glm::vec3 GetRenderAreaSize() { return renderAreaSize_; } 
        static void SetRenderAreaSize(glm::vec3 renderAreaSize);
        static glm::vec3 GetRenderAreaPos() { return renderAreaPos_; } 
        static void SetRenderAreaPos(glm::vec3 position);
        
    private:
        static glm::vec3 renderAreaSize_;
        static glm::vec3 renderAreaPos_;
    };
}

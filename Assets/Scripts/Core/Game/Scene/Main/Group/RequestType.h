#pragma once
#include <typeindex>

namespace GameCore::Scene::Main
{
    struct SceneChangeRequest final
    {
        explicit SceneChangeRequest(std::type_index type);
        
        const std::type_index type;
    };

    inline SceneChangeRequest::SceneChangeRequest(std::type_index type)
        : type(type)
    {
    }
}

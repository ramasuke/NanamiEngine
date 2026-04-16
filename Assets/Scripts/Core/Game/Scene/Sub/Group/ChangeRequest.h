#pragma once
#include "../Type/SubSceneType.h"

namespace GameCore::Scene::Sub
{
    enum class ChangeRequestType
    {
        Push,
        Pop
    };

    struct ChangeRequestOption final
    {
    public:
        explicit ChangeRequestOption(
            ChangeRequestType requestType,
            SceneType sceneType);
        
        const ChangeRequestType type;
        const SceneType sceneType;
    };

    inline ChangeRequestOption::ChangeRequestOption(ChangeRequestType requestType, SceneType sceneType)
        : type(requestType), sceneType(sceneType)
    {
    }
}

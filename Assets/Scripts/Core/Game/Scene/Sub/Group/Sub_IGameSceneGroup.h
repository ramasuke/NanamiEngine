#pragma once

namespace GameCore::Scene::Sub
{
    enum class SceneType;
}

namespace GameCore::Scene::Sub
{
    class IGameSceneStack
    {
    public:
        virtual ~IGameSceneStack() = default;
        virtual void Push(const SceneType& type) = 0;
        virtual void Pop (const SceneType& type) = 0;
    };
}

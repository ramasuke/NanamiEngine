#pragma once
#include "Engine/Core/Coroutine/Task/Task.h"

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
        /**
         * @brief サブシーンを積む。既に積まれていれば何もせず true
         * @return 積めたら true。シーンファイルの破損などで失敗したら false(ログは出し済み)
         */
        virtual Coroutine::Task<bool> PushAsync(SceneType type) = 0;
        virtual void Pop (const SceneType& type) = 0;
        virtual void Clear() = 0;
    };
}

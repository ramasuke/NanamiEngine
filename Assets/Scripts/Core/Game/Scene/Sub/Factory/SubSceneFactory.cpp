#include "SubSceneFactory.h"
#include <cassert>
#include <memory>

#include "../Content/ChattingUI/ChattingUIScene.h"
#include "../Content/ChattingUI/Context/ChattingUISceneContext.h"

namespace GameCore::Scene::Sub
{
    SceneFactory::SceneFactory(std::vector<std::weak_ptr<SceneContextBase>> contexts)
        : contexts_(std::move(contexts))
    {
        Register(SceneType::ChattingUI,
            [this]
            {
                return std::make_shared<ChattingUIScene>(CatchContext<ChattingUISceneContext>());
            });
    }
    
    void SceneFactory::Register(const SceneType& type, Creator creator)
    {
        creators_[type] = std::move(creator);
    }

    std::shared_ptr<IGameScene> SceneFactory::Create(const SceneType& type) const
    {
        const auto it = creators_.find(type);
        assert(it != creators_.end() && "Scene not registered");

        return it->second();
    }
}

#include "ChattingUIScene.h"

#include "Context/ChattingUISceneContext.h"

namespace GameCore::Scene::Sub
{
    ChattingUIScene::ChattingUIScene(const std::shared_ptr<ChattingUISceneContext>& sceneContext)
        : GameSceneBase(sceneContext)
    {
        
    }

    void ChattingUIScene::DoInit()
    {
        LoadScene();
        Context().Initialize();
    }

    void ChattingUIScene::DoDispose()
    {
        
    }

    void ChattingUIScene::DoDrawGui()
    {
        
    }
}

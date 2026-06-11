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
        scene_ = LoadScene();
        Context().Initialize();
    }

    void ChattingUIScene::DoDispose()
    {
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());   
    }

    void ChattingUIScene::DoDrawGui()
    {
        
    }
}

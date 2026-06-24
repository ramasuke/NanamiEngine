#include "OtherPlayerStatusUIScene.h"

namespace GameCore::Scene::Sub
{
    OtherPlayerStatusUiScene::OtherPlayerStatusUiScene(const std::shared_ptr<OtherPlayerStatusUiSceneContext>& sceneContext)
        : GameSceneBase(sceneContext)
    {
        
    }

    void OtherPlayerStatusUiScene::DoInit()
    {
        scene_ = LoadScene();
        Context().Initialize();
    }

    void OtherPlayerStatusUiScene::DoDispose()
    {
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());   
    }

    void OtherPlayerStatusUiScene::DoDrawGui()
    {
        
    }
}

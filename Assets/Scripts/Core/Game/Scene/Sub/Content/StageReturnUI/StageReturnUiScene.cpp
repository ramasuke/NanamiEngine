#include "StageReturnUiScene.h"

namespace GameCore::Scene::Sub
{
    StageReturnUiScene::StageReturnUiScene(const std::shared_ptr<StageReturnUiSceneContext>& sceneContext)
        : GameSceneBase(sceneContext)
    {
        
    }

    void StageReturnUiScene::DoInit()
    {
        scene_ = LoadScene();
        Context().Initialize();
    }

    void StageReturnUiScene::DoDispose()
    {
        Core::Application::ApplicationBase::GameWindow()->RemoveContent(scene_.lock());   
    }

    void StageReturnUiScene::DoDrawGui()
    {
        
    }
}

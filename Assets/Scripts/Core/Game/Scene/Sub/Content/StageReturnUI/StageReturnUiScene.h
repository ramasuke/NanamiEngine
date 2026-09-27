#pragma once
#include "../../Base/Sub_GameSceneBase.h"
#include "Context/StageReturnUiSceneContext.h"

namespace GameCore::Scene::Sub
{
    class StageReturnUiScene final : public GameSceneBase<StageReturnUiSceneContext>
    {
    public:
        explicit StageReturnUiScene(const std::shared_ptr<StageReturnUiSceneContext>& sceneContext);
        [[nodiscard]] StageReturnUiSceneContext& Context() const { return SceneContext(); }

    private:
        void DoInit   () override;
        void DoDispose() override;
        void DoDrawGui() override;
        
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
    };
}

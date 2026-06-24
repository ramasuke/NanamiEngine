#pragma once
#include "../../Base/Sub_GameSceneBase.h"
#include "Context/OtherPlayerStatusUiSceneContext.h"

namespace GameCore::Scene::Sub
{
    class OtherPlayerStatusUiScene final : public GameSceneBase<OtherPlayerStatusUiSceneContext>
    {
    public:
        explicit OtherPlayerStatusUiScene(const std::shared_ptr<OtherPlayerStatusUiSceneContext>& sceneContext);
        [[nodiscard]] OtherPlayerStatusUiSceneContext& Context() const { return SceneContext(); }

    private:
        void DoInit   () override;
        void DoDispose() override;
        void DoDrawGui() override;
        
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
    };
}

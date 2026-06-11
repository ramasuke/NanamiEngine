#pragma once
#include "../../Base/Sub_GameSceneBase.h"
#include "Context/ChattingUISceneContext.h"

namespace GameCore::Scene::Sub
{
    class ChattingUIScene final : public GameSceneBase<ChattingUISceneContext> 
    {
    public:
        explicit ChattingUIScene(const std::shared_ptr<ChattingUISceneContext>& sceneContext);
        [[nodiscard]] ChattingUISceneContext& Context() const { return SceneContext(); }

    private:
        void DoInit   () override;
        void DoDispose() override;
        void DoDrawGui() override;
        
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
    };
}

#pragma once
#include "../../Base/Sub_GameSceneBase.h"
#include "Context/ChattingUISceneContext.h"

namespace GameCore::Scene::Sub
{
    class ChattingUIScene final : public GameSceneBase<ChattingUISceneContext> 
    {
    public:
        explicit ChattingUIScene    (const std::shared_ptr<ChattingUISceneContext>& sceneContext);
        [[nodiscard]] ChattingUISceneContext& Context() const { return SceneContext(); }

    protected:
        void DoInit   () override;
        void DoDispose() override;
        void DoDrawGui() override;
    };
}

#pragma once
#include "../../Base/Main_GameSceneBase.h"
#include "Context/TitleSceneContext.h"

namespace GameCore::Scene::Main
{
    class TitleScene final : public GameMainSceneBase<TitleSceneContext>
    {
    public:
        explicit TitleScene(const std::weak_ptr<TitleSceneContext>& context, GameSceneBaseContext baseContext);

    private:
        void Init     () override;
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;

    private:
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
    };
}

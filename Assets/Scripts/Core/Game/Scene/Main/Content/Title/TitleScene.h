#pragma once
#include "../../Base/Main_GameSceneBase.h"
#include "Context/TitleSceneContext.h"

namespace GameCore::Scene::Main
{
    class TitleScene final : public GameMainSceneBase<TitleSceneContext>
    {
    public:
        explicit TitleScene(const std::weak_ptr<TitleSceneContext>& coSntext, GameSceneBaseContext baseContext);

    private:
        void Init     () override;
        void OnEnter  () override;
        void OnExit   () override;
        void Dispose  () override;
        void OnDrawGui() override;

    private:
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
    };
}

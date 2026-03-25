#pragma once
#include "../Base/GameSceneBase.h"
#include "Context/TitleSceneContext.h"

namespace GameCore::Scene
{
    class TitleScene final : public GameSceneBase<TitleSceneContext>
    {
    public:
        explicit TitleScene(const std::weak_ptr<TitleSceneContext>& coSntext, GameSceneBaseContext baseContext);

    private:
        void Init()      override;
        void OnEnter() override;
        void OnExit() override;
        void Dispose()   override;
        void OnDrawGui() override;

    private:
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
    };
}

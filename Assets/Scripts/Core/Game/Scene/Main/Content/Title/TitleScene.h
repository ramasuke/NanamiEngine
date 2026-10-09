#pragma once
#include "../../Base/Main_GameSceneBase.h"
#include "Context/TitleSceneContext.h"

namespace GameCore::Scene::Main
{
    class TitleScene final : public GameMainSceneBase<TitleSceneContext>
    {
    public:
        explicit TitleScene(
            const std::weak_ptr<TitleSceneContext>& context,
            GameSceneBaseContext baseContext);

    private:
        void OnInit() override;
        [[nodiscard]] std::vector<Sub::SceneType> SubScenes() const override { return {}; }
        Coroutine::Task<EnterResult> OnEnterAsync(NanamiEngine::R4::CancellationToken token) override;
        void OnEntered() override {}
        void Enter    () override;
        void DoExit() override;
        void OnDrawGui() override;
        // NOTE: タイトルが読めなければ逃げ場が無いので、そのまま画面を明ける
        [[nodiscard]] std::optional<SceneType> FallbackSceneOnFailure() const override { return std::nullopt; }
    };
}

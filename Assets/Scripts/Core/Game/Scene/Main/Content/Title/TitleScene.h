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
        void OnInit() override;
        Coroutine::Task<EnterResult> OnEnterAsync(NanamiEngine::R4::CancellationToken token) override;
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;
        /** @brief タイトルが読めなければ逃げ場が無いので、そのまま画面を明ける */
        [[nodiscard]] std::optional<SceneType> FallbackSceneOnFailure() const override { return std::nullopt; }
    };
}

#pragma once
#include <memory>

#include "../../Base/Main_GameSceneBase.h"
#include "Context/DragonNestSceneContext.h"
#include "Packages/R4/R4.h"

namespace GameCore::Scene::GrassLand
{
    template<class TContext>
    class StageArrivalMovie;
}

namespace GameCore::Scene::Main
{
    /** 古竜の巣 (SceneType::DragonNest)。嵐の目に浮かぶ竜の墓場 (docs/Story.md 終章「嵐の巣」) */
    class DragonNestScene final : public GameMainSceneBase<DragonNestSceneContext>
    {
    public:
        explicit DragonNestScene(
            const std::weak_ptr<DragonNestSceneContext>& context,
            GameSceneBaseContext baseContext);
        ~DragonNestScene() override;

    private:
        void OnInit() override;
        [[nodiscard]] std::vector<Sub::SceneType> SubScenes() const override;
        Coroutine::Task<EnterResult> OnEnterAsync(NanamiEngine::R4::CancellationToken token) override;
        void OnEntered() override;
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;
        void OnStageClear(Story::StoryFlag flag);

        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::shared_ptr<GrassLand::StageArrivalMovie<DragonNestSceneContext>> arrivalMovie_;
        Story::StageClearWatcher stageClearWatcher_;
        /** @brief このステージでボスを倒したか。抜けるときに体力を満タンにして保存する */
        bool isStageCleared_ = false;
    };
}

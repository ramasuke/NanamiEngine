#pragma once
#include <memory>

#include "../../Base/Main_GameSceneBase.h"
#include "Context/DrySandSceneContext.h"
#include "Packages/R4/R4.h"

namespace GameCore::Scene::GrassLand
{
    template<class TContext>
    class StageArrivalMovie;
}

namespace GameCore::Scene::Main
{
    /** 砂漠地帯 (SceneType::Desert)。オアシスの隊商と、砂に沈んだ城塞の骸竜 (docs/Story.md 第2章) */
    class DrySandScene final : public GameMainSceneBase<DrySandSceneContext>
    {
    public:
        explicit DrySandScene(
            const std::weak_ptr<DrySandSceneContext>& context,
            GameSceneBaseContext baseContext);
        ~DrySandScene() override;

    private:
        void Init     () override;
        /** @param generation Dispose を跨いだ古いコルーチンを弾くための世代番号 */
        Coroutine::Task<void> OnEnterAsync(int generation);
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;
        /** @brief 骸竜を倒したら、神殿前の広場の光の浮遊石が空へ飛び去る */
        void OnStageClear(Story::StoryFlag flag);

        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::shared_ptr<GrassLand::StageArrivalMovie<DrySandSceneContext>> arrivalMovie_;
        NanamiEngine::R4::Disposable stageClearSubscription_;
    };
}

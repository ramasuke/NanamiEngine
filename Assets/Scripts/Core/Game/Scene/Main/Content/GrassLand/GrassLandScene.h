#pragma once
#include <memory>

#include "../../Base/Main_GameSceneBase.h"
#include "Context/GrassLandSceneContext.h"
#include "Packages/R4/R4.h"

namespace GameCore::Scene::GrassLand
{
    template<class TContext>
    class StageArrivalMovie;
}

namespace GameCore::Scene::Main
{
    class GrassLandScene final : public GameMainSceneBase<GrassLandSceneContext>
    {
    public:
        explicit GrassLandScene(
            const std::weak_ptr<GrassLandSceneContext>& context,
            GameSceneBaseContext baseContext);
        ~GrassLandScene() override;
        
    private:
        void Init     () override;
        /** @param generation Dispose を跨いだ古いコルーチンを弾くための世代番号 */
        Coroutine::Task<void> OnEnterAsync(int generation);
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;
        /** @brief 大顎を倒したら、村の跡の浮遊石が空へ飛び去る */
        void OnStageClear(Story::StoryFlag flag);
        
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::shared_ptr<GrassLand::StageArrivalMovie<GrassLandSceneContext>> arrivalMovie_;
        NanamiEngine::R4::Disposable stageClearSubscription_;
        /** シーンを抜けたら立てて、浮遊石の演出を止める */
        std::shared_ptr<bool> isStoneMovieCanceled_ = std::make_shared<bool>(false);
    };
}

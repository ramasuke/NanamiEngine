#pragma once
#include <memory>

#include "../../Base/Main_GameSceneBase.h"
#include "Context/GrassLandSceneContext.h"

namespace GameCore::Scene::GrassLand
{
    class GrassLandArrivalMovie;
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
        Coroutine::Task<void> BackToMainIslandAsync(int generation);
        void Enter    () override;
        void DoDispose() override;
        void OnDrawGui() override;
        
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::shared_ptr<GrassLand::GrassLandArrivalMovie> arrivalMovie_;
        int loadGeneration_ = 0;
    };
}

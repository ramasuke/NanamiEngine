#pragma once
#include <memory>

#include "../../Base/Main_GameSceneBase.h"
#include "Context/GrassLandSceneContext.h"

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
        void Enter    () override;
        void Dispose  () override;
        void OnDrawGui() override;
        
        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
    };
}

#pragma once
#include "../../Base/Main_GameSceneBase.h"
#include "Context/MainIsLandSceneContext.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::Scene::Main
{
    class MainIslandScene final : public GameMainSceneBase<MainIslandSceneContext>
    {
    public:
        explicit MainIslandScene(const std::weak_ptr<MainIslandSceneContext>& context, GameSceneBaseContext baseContext);
        ~MainIslandScene() override;
        
    private:
        void Init     () override;
        void OnEnter  () override;
        void OnExit   () override;
        void Dispose  () override;
        void OnDrawGui() override;

        std::weak_ptr<NanamiEngine::Scene::Scene> scene_;
        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> playerAvatar_;
    };
}

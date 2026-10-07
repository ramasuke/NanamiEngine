#pragma once
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::Scene
{
    class FirstTouchDownMainIsLandSceneContext;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    class AboardAirShipMovie final : public std::enable_shared_from_this<AboardAirShipMovie>
    {
    public:
        explicit AboardAirShipMovie(
              const std::weak_ptr<IPlayerAvatar>& playerAvatar
            , const std::shared_ptr<FirstTouchDownMainIsLandSceneContext>& context);

        static Coroutine::Task<void> PlayAsync(std::shared_ptr<AboardAirShipMovie> movie);
        /** @brief 演出を流さず、船が着いた後の状態にする。主人公を出す前に呼ぶ */
        static void DockImmediately(FirstTouchDownMainIsLandSceneContext& context);
        void Cancel() { isCancelled_ = true; }

    private:
        Coroutine::Task<void> Invoke();
        static Coroutine::Task<void> StagingAsync(std::shared_ptr<AboardAirShipMovie> movie);
        [[nodiscard]] bool ShouldStop() const;
        [[nodiscard]] std::shared_ptr<FirstTouchDownMainIsLandSceneContext> Context() const { return context_.lock(); }
        Coroutine::Task<void> AboardAirShipMovieMoveAirShipAsync();
        Coroutine::Task<void> AirShipMovieStagingAsync          ();
        Coroutine::Task<void> AirShipMovieOpeningShotsAsync     ();
        Coroutine::Task<void> AirShipMovieOpeningShotAsync      (std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> shot, float duration_secs);
        Coroutine::Task<void> AirShipMovieJoinPlayerCameraShotAsync(std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> shot, float duration_secs);
        void FadeOutTitleLogo() const;
        static void LoosenDeckProps(FirstTouchDownMainIsLandSceneContext& context);

        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::weak_ptr<FirstTouchDownMainIsLandSceneContext> context_;
        bool isCancelled_       = false;
        bool isOpeningFinished_ = false;
    };
}

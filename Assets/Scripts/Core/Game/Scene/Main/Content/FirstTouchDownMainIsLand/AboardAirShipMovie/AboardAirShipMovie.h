#pragma once
#include "../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"

namespace GameCore::Scene
{
    class FirstTouchDownMainIsLandSceneContext;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore::Scene::FirstTouchDownMainIsLand
{
    /** @brief AboardAirShipMovieに関連する処理を行うクラス*/
    class AboardAirShipMovie final
    {
    public:
        explicit AboardAirShipMovie(
              const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& playerAvatar
            , const std::shared_ptr<FirstTouchDownMainIsLandSceneContext>& context);

        Coroutine::Task<void> ToTask() { co_await Invoke(); }

    private:
        Coroutine::Task<void> Invoke();
        [[nodiscard]] std::shared_ptr<FirstTouchDownMainIsLandSceneContext> Context() const { return context_.lock(); }
        Coroutine::Task<void> AboardAirShipMovieMoveAirShipAsync();
        Coroutine::Task<void> AirShipMovieStagingAsync          ();
        Coroutine::Task<void> AirShipMovieFirstCameraMoveAsync  ();
        Coroutine::Task<void> AirShipMovieWalkPlayerAsync       ();
        Coroutine::Task<void> AirShipMovieArmStretchPlayerAsync ();
        void StartFadeInUi() const;

        //無駄
        // Coroutine::Task<void> AboardAirShipMovie::ArmStretchAsync() const;
        
        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> playerAvatar_;
        std::weak_ptr<FirstTouchDownMainIsLandSceneContext> context_;
    };
}

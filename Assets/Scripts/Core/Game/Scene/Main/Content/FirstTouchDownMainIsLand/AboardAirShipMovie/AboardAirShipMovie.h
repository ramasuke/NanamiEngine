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
    /** @brief AboardAirShipMovieに関連する処理を行うクラス */
    class AboardAirShipMovie final : public std::enable_shared_from_this<AboardAirShipMovie>
    {
    public:
        explicit AboardAirShipMovie(
              const std::weak_ptr<IPlayerAvatar>& playerAvatar
            , const std::shared_ptr<FirstTouchDownMainIsLandSceneContext>& context);

        /** @brief ムービーを流す。コルーチンが走っている間は movie を持ち続ける */
        static Coroutine::Task<void> PlayAsync(std::shared_ptr<AboardAirShipMovie> movie);
        /** @brief シーンを抜けるときに呼ぶ。コルーチンは止められないので、次の区切りで抜ける */
        void Cancel() { isCancelled_ = true; }

    private:
        Coroutine::Task<void> Invoke();
        static Coroutine::Task<void> StagingAsync(std::shared_ptr<AboardAirShipMovie> movie);
        /** @brief シーンを抜けた、またはアバター・シーンの Context がもう居ない */
        [[nodiscard]] bool ShouldStop() const;
        [[nodiscard]] std::shared_ptr<FirstTouchDownMainIsLandSceneContext> Context() const { return context_.lock(); }
        Coroutine::Task<void> AboardAirShipMovieMoveAirShipAsync();
        Coroutine::Task<void> AirShipMovieStagingAsync          ();
        Coroutine::Task<void> AirShipMovieOpeningShotsAsync     ();
        /** @brief shot の VirtualCamera に切り替え、duration_secs かけて最初の子の位置・向きへ動かす */
        Coroutine::Task<void> AirShipMovieOpeningShotAsync      (std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> shot, float duration_secs);
        void FadeOutTitleLogo() const;
        /** @brief 甲板の小物を Dynamic にして、押したり転がしたりできるようにする */
        void LoosenDeckProps() const;

        //無駄
        // Coroutine::Task<void> AboardAirShipMovie::ArmStretchAsync() const;
        
        std::weak_ptr<IPlayerAvatar> playerAvatar_;
        std::weak_ptr<FirstTouchDownMainIsLandSceneContext> context_;
        bool isCancelled_       = false;
        bool isOpeningFinished_ = false; ///< プレイヤーに操作を返したか。船はそれから動き出す
    };
}

#pragma once
#include <memory>

#include "../../../../../../../../../Engine/Core/Coroutine/Task/Task.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::Scene
{
    class GrassLandSceneContext;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Scene::GrassLand
{
    /**
     * @brief GrassLandに着いたときの、ポータルからせり上がってくる登場演出
     * @note ローカルクライアントでしか走らず、RPCも送らないので他プレイヤーには見えない
     */
    class GrassLandArrivalMovie final
    {
    public:
        explicit GrassLandArrivalMovie(
              const std::weak_ptr<IPlayerAvatar>& playerAvatar
            , const std::shared_ptr<GrassLandSceneContext>& context);

        /** @brief ロード画面が明ける前に呼ぶ。ポータルを出し、カメラを寄せ、プレイヤーをWarpInにする */
        void Begin();
        /** @brief シーンを畳むときに呼ぶ。走っているコルーチンを次のawaitで抜けさせる */
        void Cancel() { isCanceled_ = true; }

        /**
         * @brief カメラを動かし、終わったら三人称へ返す
         * @param self コルーチンが走っている間の生存を保証するための自分自身
         */
        static Coroutine::Task<void> PlayAsync(std::shared_ptr<GrassLandArrivalMovie> self);

    private:
        /** @param rate 0でショットの始点、1で終点 */
        void ApplyShot(float rate) const;
        void Finish(bool isSkipped);

        std::weak_ptr<IPlayerAvatar>         playerAvatar_;
        std::weak_ptr<GrassLandSceneContext> context_;
        std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject> portal_;
        bool isCanceled_ = false;
        bool isFinished_ = false;
    };
}

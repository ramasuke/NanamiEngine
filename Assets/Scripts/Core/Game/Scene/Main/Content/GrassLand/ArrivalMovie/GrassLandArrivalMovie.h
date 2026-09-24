#pragma once
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "Libs/glm/vec3.hpp"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::CineMachine::Behaviour
{
    class VirtualCameraFollowBehaviour;
    class VirtualCameraLookAtBehaviour;
}

namespace GameCore::Scene::GrassLand
{
    /**
     * @brief 狩り場に着いたときの、ポータルから歩いて出てくる演出
     * @tparam TContext 到着演出の設定 (ArrivalCamera / ArrivalPortalPrefab / Arrival*_msecs など) を持つシーンのコンテキスト。
     *                  草原と砂漠で使うので、.cpp で両方を明示的に実体化している
     */
    template<class TContext>
    class StageArrivalMovie final
    {
    public:
        explicit StageArrivalMovie(
              const std::weak_ptr<IPlayerAvatar>& playerAvatar
            , const std::shared_ptr<TContext>& context);
        
        void Begin();
        void Cancel() { isCanceled_ = true; }

        /**
         * @brief ポータルを開き、プレイヤーを歩かせてカメラで見上げ、終わったら三人称へ返す
         * @param self コルーチンが走っている間の生存を保証するための自分自身
         */
        static Coroutine::Task<void> PlayAsync(std::shared_ptr<StageArrivalMovie> self);

    private:
        /** @param rate 0で膜の奥の歩き出す位置、1で立ち止まる位置 */
        [[nodiscard]] glm::vec3 WalkPos(float rate) const;
        [[nodiscard]] glm::vec3 PortalCenter() const;
        void DestroyPortal();
        void SetAvatarVisible(bool isVisible) const;
        /** @brief ポータルを片付けてカメラを返す。歩き終える前にスキップされたときは、立ち止まる位置へ送ってから操作を返す */
        void Finish();

        std::weak_ptr<IPlayerAvatar>         playerAvatar_;
        std::weak_ptr<TContext>              context_;
        std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject> portal_;
        std::weak_ptr<NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour> cameraFollow_;
        std::weak_ptr<NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour> cameraLookAt_;
        glm::vec3 portalScale_    = glm::vec3(1.0f);              ///< プレハブのルートのスケール。開ききったときの大きさ
        glm::vec3 groundPos_      = glm::vec3(0.0f);              ///< ポータルの足元の地面。演出の位置はすべてここから取る
        glm::vec3 forward_        = glm::vec3(0.0f, 0.0f, -1.0f); ///< ポータルから歩いて出ていく水平方向
        glm::vec3 side_           = glm::vec3(1.0f, 0.0f,  0.0f); ///< forward_ に直交する水平方向。カメラの横位置の軸
        glm::vec3 cameraStartPos_ = glm::vec3(0.0f);
        glm::vec3 cameraEndPos_   = glm::vec3(0.0f);
        bool isBegun_        = false; ///< Beginで演出の準備が済んだか。プレイヤーが居なければ何もしない
        bool isWalkFinished_ = false;
        bool isCanceled_     = false;
        bool isFinished_     = false;
    };
}

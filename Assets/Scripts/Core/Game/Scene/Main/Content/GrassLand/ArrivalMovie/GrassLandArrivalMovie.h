#pragma once
#include <memory>
#include <optional>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "Libs/glm/vec3.hpp"
#include "Packages/ControlLock/ControlLock.h"
#include "../../../../../Story/Story_StoryFlag.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GamePlay::Ui
{
    class StageArrivalCaption;
}

namespace NanamiEngine::CineMachine
{
    class CineMachineVirtualCamera;
}

namespace NanamiEngine::CineMachine::Behaviour
{
    class VirtualCameraFollowBehaviour;
    class VirtualCameraLookAtBehaviour;
}

namespace GameCore::Scene::GrassLand
{
    template<class TContext>
    class StageArrivalMovie final
    {
    public:
        explicit StageArrivalMovie(
              const std::weak_ptr<IPlayerAvatar>& playerAvatar
            , const std::shared_ptr<TContext>& context
            , std::optional<Story::StoryFlag> overviewSeenFlag);
        
        void Begin();
        void Cancel() { isCanceled_ = true; }

        /**
         * @brief (初めて着いたときだけ) 島を見下ろして見どころを巡ってからポータルの前へ降り、ポータルを開き、プレイヤーを歩かせてカメラで見上げ、終わったら三人称へ返す
         * @param self コルーチンが走っている間の生存を保証するための自分自身
         */
        static Coroutine::Task<void> PlayAsync(std::shared_ptr<StageArrivalMovie> self);

    private:
        /** @param rate 0で膜の奥の歩き出す位置、1で立ち止まる位置 */
        [[nodiscard]] glm::vec3 WalkPos(float rate) const;
        [[nodiscard]] glm::vec3 PortalCenter() const;
        void DestroyPortal();
        void SetAvatarVisible(bool isVisible) const;
        /** @return スキップされたか。空撮しない設定か、もう見ていれば何もせず false */
        static Coroutine::Task<bool> PlayOverviewAsync(std::shared_ptr<StageArrivalMovie> self);
        /**
         * @brief start へ切ってから、end へ尺をかけて Brain の補間で動く1ショット。終わったら2台とも下ろす
         * @return スキップされたか
         */
        static Coroutine::Task<bool> PlayShotAsync(
              std::shared_ptr<StageArrivalMovie> self
            , std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> start
            , std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> end
            , int duration_msecs);
        /** @return スキップされたか */
        static Coroutine::Task<bool> WaitShotAsync(std::shared_ptr<StageArrivalMovie> self, int duration_msecs);
        /** @brief 空撮と見どころのカメラを下ろす */
        void DisableShotCameras() const;
        /** @brief 補間せずにカメラを pos に置き、lookAt を向かせる (ショットの切り替え) */
        void SnapCamera(const glm::vec3& pos, const glm::vec3& lookAt) const;
        void MarkOverviewSeen() const;
        void ReleaseCaption();
        /** @brief 押しっぱなしで来た入力では反応しないよう、一度離してから押されたときだけ true */
        bool IsSkipRequested();
        /** @brief ポータルを片付けてカメラを返す。歩き終える前にスキップされたときは、立ち止まる位置へ送ってから操作を返す */
        void Finish();

        NanamiEngine::ControlLock::ScopedLock controlLock_;
        std::weak_ptr<IPlayerAvatar>         playerAvatar_;
        std::weak_ptr<TContext>              context_;
        std::optional<Story::StoryFlag>      overviewSeenFlag_;
        std::weak_ptr<NanamiEngine::Module::GameObject::IGameObject> portal_;
        std::weak_ptr<GamePlay::Ui::StageArrivalCaption> caption_;
        std::weak_ptr<NanamiEngine::CineMachine::Behaviour::VirtualCameraFollowBehaviour> cameraFollow_;
        std::weak_ptr<NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour> cameraLookAt_;
        glm::vec3 portalScale_    = glm::vec3(1.0f);        
        glm::vec3 groundPos_      = glm::vec3(0.0f);        
        glm::vec3 forward_        = glm::vec3(0.0f, 0.0f, -1.0f); 
        glm::vec3 side_           = glm::vec3(1.0f, 0.0f,  0.0f); 
        glm::vec3 cameraStartPos_ = glm::vec3(0.0f);
        glm::vec3 cameraEndPos_   = glm::vec3(0.0f);
        bool isBegun_        = false; 
        bool hasOverview_    = false; 
        bool isSkipArmed_    = false;
        bool isWalkFinished_ = false;
        bool isCanceled_     = false;
        bool isFinished_     = false;
    };
}

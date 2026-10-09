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
        
        static Coroutine::Task<void> PlayAsync(std::shared_ptr<StageArrivalMovie> self);

    private:
        [[nodiscard]] glm::vec3 WalkPos(float rate) const;
        [[nodiscard]] glm::vec3 PortalCenter() const;
        void DestroyPortal();
        void SetAvatarVisible(bool isVisible) const;
        static Coroutine::Task<bool> PlayOverviewAsync(std::shared_ptr<StageArrivalMovie> self);
        static Coroutine::Task<bool> PlayShotAsync(
              std::shared_ptr<StageArrivalMovie> self
            , std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> start
            , std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> end
            , int duration_msecs);
        static Coroutine::Task<bool> WaitShotAsync(std::shared_ptr<StageArrivalMovie> self, int duration_msecs);
        void DisableShotCameras() const;
        // NOTE: 補間せずにカメラを pos に置き、lookAt を向かせる
        void SnapCamera(const glm::vec3& pos, const glm::vec3& lookAt) const;
        void MarkOverviewSeen() const;
        void ReleaseCaption();
        bool IsSkipRequested();
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

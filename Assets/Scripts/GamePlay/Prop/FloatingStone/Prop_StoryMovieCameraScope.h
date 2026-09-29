#pragma once
#include <memory>

#include "Libs/glm/vec3.hpp"
#include "Packages/ControlLock/ControlLock.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::CineMachine
{
    class CineMachineVirtualCamera;
}

namespace GamePlay::Prop::StoryMovie
{
    /** @brief カメラとプレイヤーの操作を演出のあいだだけ借りる */
    class CameraScope final
    {
    public:
        CameraScope(
            std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar,
            std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera,
            std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> lookTarget,
            const glm::vec3& lookOffset);
        ~CameraScope();

        void Begin();
        void End();

    private:
        NanamiEngine::ControlLock::ScopedLock controlLock_;
        std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar_;
        std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera_;
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> lookTarget_;
        glm::vec3 lookOffset_;
        bool isEnded_ = false;
    };
}

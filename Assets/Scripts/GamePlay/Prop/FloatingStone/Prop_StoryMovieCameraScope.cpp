#include "Prop_StoryMovieCameraScope.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"

namespace GamePlay::Prop::StoryMovie
{
    namespace
    {
        // NOTE: 到着演出や NPC の会話カメラより上に出す
        constexpr int CAMERA_PRIORITY = 110;
    }

    CameraScope::CameraScope(
        std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar,
        std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> camera,
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> lookTarget,
        const glm::vec3& lookOffset)
        : playerAvatar_(std::move(playerAvatar))
        , camera_(std::move(camera))
        , lookTarget_(std::move(lookTarget))
        , lookOffset_(lookOffset)
    {
    }

    CameraScope::~CameraScope() { End(); }

    void CameraScope::Begin()
    {
        if (!playerAvatar_.expired())
            controlLock_.Set(NanamiEngine::ControlLock::Service::Instance().Acquire());

        if (!camera_)
            return;

        const auto lookAt = camera_->Components().Catch<NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour>().lock();
        if (lookAt && lookTarget_)
        {
            lookAt->SetTarget(lookTarget_);
            lookAt->SetOffsetPos(lookOffset_);
        }
        camera_->SetPriority(CAMERA_PRIORITY);
    }

    void CameraScope::End()
    {
        if (isEnded_)
            return;
        isEnded_ = true;

        // NOTE: カメラを無効にすると三人称カメラが勝ち、Brain のブレンドで帰る
        if (camera_)
            camera_->OnDisable();
        controlLock_.Release();
    }
}

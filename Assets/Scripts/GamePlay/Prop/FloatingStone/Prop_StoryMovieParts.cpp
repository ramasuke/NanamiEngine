#include "Prop_StoryMovieParts.h"

#include "DxLib.h"

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Physics/Physics.h"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"

namespace GamePlay::Prop::StoryMovie
{
    namespace
    {
        // 到着演出(100)や NPC の会話カメラより上に出す
        constexpr int CAMERA_PRIORITY = 110;
        constexpr unsigned char SKIP_TRIGGER_DEAD_ZONE = 30;

        bool IsSkipInputDown()
        {
            if (CheckHitKeyAll(DX_CHECKINPUT_KEY) != 0)
                return true;

            XINPUT_STATE xInput = {};
            if (GetJoypadXInputState(DX_INPUT_PAD1, &xInput) != 0)
                return false;

            if (xInput.LeftTrigger > SKIP_TRIGGER_DEAD_ZONE || xInput.RightTrigger > SKIP_TRIGGER_DEAD_ZONE)
                return true;

            for (const auto button : xInput.Buttons)
            {
                if (button != 0)
                    return true;
            }
            return false;
        }
    }

    bool SkipInput::IsSkipped()
    {
        const bool isDown = IsSkipInputDown();
        isArmed_ |= !isDown;
        return isArmed_ && isDown;
    }

    void SetChildParticlesPlaying(NanamiEngine::Module::GameObject::IGameObject& root, const bool isPlaying)
    {
        for (const auto& child : root.Transform().GetAllChildren())
        {
            if (const auto particle = child->Components().Catch<NanamiEngine::Module::Component::ParticleSystem>().lock())
            {
                if (isPlaying)
                    particle->Play();
                else
                    particle->Stop();
            }
        }
    }

    void RebuildColliders(NanamiEngine::Module::GameObject::IGameObject& root)
    {
        auto& bodies = NanamiEngine::Core::Application::ApplicationBase::Physics().Bodies();
        const auto rebuild = [&bodies](NanamiEngine::Module::GameObject::IGameObject& gameObject)
        {
            for (const auto& weak : gameObject.Components().Catches<NanamiEngine::Module::Component::ColliderBase>())
            {
                if (const auto collider = weak.lock())
                    bodies.MarkDirty(*collider);
            }
        };
        rebuild(root);
        for (const auto& child : root.Transform().GetAllChildren())
            rebuild(*child);
    }

    void MoveBy(NanamiEngine::Module::GameObject::IGameObject& gameObject, const glm::vec3& offset)
    {
        gameObject.Transform().SetWorldPos(gameObject.Transform().GetWorldPos() + offset);
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

    void CameraScope::Begin() const
    {
        if (const auto avatar = playerAvatar_.lock())
            avatar->GetEventSceneStateMachine().OnDisable();

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

        // 優先度を戻すと三人称カメラが勝ち、Brain のブレンドで帰る
        if (camera_)
            camera_->OnDisable();
        if (const auto avatar = playerAvatar_.lock())
            avatar->GetEventSceneStateMachine().OnEnable();
    }
}

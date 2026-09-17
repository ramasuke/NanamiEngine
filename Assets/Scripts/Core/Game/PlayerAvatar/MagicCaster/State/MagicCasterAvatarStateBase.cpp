#include "MagicCasterAvatarStateBase.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Engine/Module/Component/Animator/Animator.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    MagicCasterAvatarStateBase::MagicCasterAvatarStateBase(
        const std::shared_ptr<MagicCasterAvatarStateContext>& context
        , const std::function<void(MagicCasterAvatarStateType)>& onChangeState)
        : stateDuring_secs_(0.0f)
        , context_         (context)
        , onChangeState_   (onChangeState)
    {
    }

    void MagicCasterAvatarStateBase::OnEnter()
    {
        ResetDuringTime();
        DoEnter();
    }

    void MagicCasterAvatarStateBase::OnUpdate()
    {
        DoUpdate();
        stateDuring_secs_ += Time::DeltaTime();
    }

    void MagicCasterAvatarStateBase::OnFixedUpdate()
    {
        DoFixedUpdate();
    }

    void MagicCasterAvatarStateBase::OnExit()
    {
        DoExit();
    }

    Component::Animator& MagicCasterAvatarStateBase::Animator() const
    {
        return *Player().Components().Catch<Component::Animator>().lock();
    }

    void MagicCasterAvatarStateBase::ResetDuringTime()
    {
        stateDuring_secs_ = 0.0f;
    }

    void MagicCasterAvatarStateBase::HoldHorizontalVelocity() const
    {
        RigidBody().SetLinearVelocity(glm::vec3(0.0f, RigidBody().LinearVelocity().y, 0.0f));
    }

    void MagicCasterAvatarStateBase::ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const
    {
        CameraGroup().ChangeCamera(camera);
    }

    void MagicCasterAvatarStateBase::OnChangeState(MagicCasterAvatarStateType type) const
    {
        onChangeState_(type);
    }
}

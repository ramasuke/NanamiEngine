#include "SwordManAvatarStateBase.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../GamePlay/PlayerAvatar/ChattableArea/ChattableArea.h"
#include "../../Chattable/IPlayerChattable.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStateBase::SwordManAvatarStateBase(
        const std::shared_ptr<SwordManAvatarStateContext>& context
        , const std::function<void(std::type_index)>& onChangeState)
        : stateDuring_secs_(0.0f            )
        , context_         (context         )
        , onChangeState_   (onChangeState)
    {
    }
    
    void SwordManAvatarStateBase::OnEnter()
    {
        ResetDuringTime();
        DoEnter();
    }
    
    void SwordManAvatarStateBase::OnUpdate()
    {
        DoUpdate();
        stateDuring_secs_ += Time::DeltaTime();
    }
    
    void SwordManAvatarStateBase::OnExit()
    {
        DoExit();
    }

    Component::Animator& SwordManAvatarStateBase::Animator() const
    {
        return *Player().Components().Catch<Component::Animator>().lock();
    }

    void SwordManAvatarStateBase::ResetDuringTime()
    {
        stateDuring_secs_ = 0.0f;
    }

    void SwordManAvatarStateBase::ChangeCamera(const std::weak_ptr<CineMachine::CineMachineVirtualCamera>& camera) const
    {
        CameraGroup().ChangeCamera(camera);
    }
}

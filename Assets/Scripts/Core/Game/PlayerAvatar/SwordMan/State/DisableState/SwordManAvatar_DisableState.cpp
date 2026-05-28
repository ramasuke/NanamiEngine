#include "SwordManAvatar_DisableState.h"

#include "../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"

void GameCore::PlayerAvatar::SwordMan::State::DisableState::DoEnter()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::DisableState::DoUpdate()
{
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
}

void GameCore::PlayerAvatar::SwordMan::State::DisableState::DoExit()
{
}

GameCore::PlayerAvatar::SwordMan::AnimationType GameCore::PlayerAvatar::SwordMan::State::DisableState::
AnimationType() const
{
    if (static_cast<enum AnimationType>(Animator().Param<int>("State").Get()) == AnimationType::Walk)
    {
        return AnimationType::Walk;
    }
    return AnimationType::Idle;
}

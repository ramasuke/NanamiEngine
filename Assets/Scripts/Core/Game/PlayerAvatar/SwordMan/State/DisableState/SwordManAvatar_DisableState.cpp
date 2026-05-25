#include "SwordManAvatar_DisableState.h"

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

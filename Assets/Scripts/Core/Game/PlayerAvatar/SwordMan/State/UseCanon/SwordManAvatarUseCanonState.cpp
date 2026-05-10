#include "SwordManAvatarUseCanonState.h"

#include "../../../../Game.h"
#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoEnter()
{
    const auto& sceneContext = *Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>();
    sceneContext.PlayerControllabeCanon().Use();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoUpdate()
{
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
    
    const auto& sceneContext = *Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>();
    auto& cannon = sceneContext.PlayerControllabeCanon();
    
    if (Input().Move().ReadValue().x > 0.0f)
    {
        cannon.RightRotate();
    }
    else if (Input().Move().ReadValue().x < 0.0f)
    {
        cannon.LeftRotate();
    }
    if (Input().CannonAttack().IsPressed())
    {
        cannon.Shoot();
    }
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoExit()
{
    
}

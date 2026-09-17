#include "SwordManAvatarUseCanonState.h"

#include "../../../../Game.h"
#include "../../../../Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoEnter()
{
    const auto& sceneContext = *Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>();
    sceneContext.PlayerControllabeCanon().Use();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
    
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

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoUpdate()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoExit()
{

}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
{
    visitor.Action(SwordManAvatarStateAction::CannonTurn, true);
    visitor.Action(SwordManAvatarStateAction::CannonFire, true);
}

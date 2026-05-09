#include "SwordManAvatarUseCanonState.h"

#include "../../../../Game.h"
#include "../../../../Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../../../Scene/Main/Group/Main_GameSceneGroup.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoEnter()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoUpdate()
{
    const auto& sceneContext = *Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>();
    if (Input().Move().ReadValue().x > 0.0f)
    {
        sceneContext.PlayerControllabeCanon().RightRotate();
    }
    else if (Input().Move().ReadValue().x < 0.0f)
    {
        sceneContext.PlayerControllabeCanon().LeftRotate();
    }
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarUseCannonState::DoExit()
{
    
}

#include "SwordManAvatar_DeathState.h"

#include "../../../../Game.h"
#include "../../../../Scene/Group/Main/GameMainSceneGroup.h"
#include "../../../../Scene/Title/TitleScene.h"

void GameCore::PlayerAvatar::SwordMan::State::DeathState::DoEnter()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::DeathState::DoUpdate()
{
    if (During_secs() > Status().DeathStateDuration_secs())
    {
        Game::Instance().MainScene().RequestChangeScene<Scene::TitleScene>();
    }
}

void GameCore::PlayerAvatar::SwordMan::State::DeathState::DoExit()
{
    
}

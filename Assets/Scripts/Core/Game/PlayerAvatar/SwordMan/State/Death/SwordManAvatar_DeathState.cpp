#include "SwordManAvatar_DeathState.h"

#include "../../../../Game.h"
#include "../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../Scene/Main/Content/Title/TitleScene.h"

void GameCore::PlayerAvatar::SwordMan::State::DeathState::DoEnter()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::DeathState::DoUpdate()
{
    if (During_secs() > Status().DeathStateDuration_secs())
    {
        // Game::Instance().Scenes().RequestChangeScene<Scene::Main::TitleScene>();
    }
}

void GameCore::PlayerAvatar::SwordMan::State::DeathState::DoExit()
{
    
}

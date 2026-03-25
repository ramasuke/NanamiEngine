#include "SceneContextBase.h"

#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

void GameCore::Scene::SceneContextBase::Init()
{
    playerSpawnPoint_.InitForPrompty();
}

glm::vec3 GameCore::Scene::SceneContextBase::PlayerSpawnPoint() const
{
    return playerSpawnPoint_->TransformRef().GetWorldPos();
}

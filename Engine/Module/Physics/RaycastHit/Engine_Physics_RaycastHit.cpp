#include "Engine_Physics_RaycastHit.h"

#include <memory>

NanamiEngine::Module::Physics::RaycastHit::RaycastHit(
    const bool hit,
    const glm::vec3& position,
    const glm::vec3& normal,
    const std::weak_ptr<GameObject::IGameObject>& hitObject)
    : hit_(hit),
      position_(position),
      normal_(normal),
      hitObject_(hitObject)
{
}

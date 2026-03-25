#include "RaycastHit.h"

NanamiEngine::Module::Physics::RaycastHit::RaycastHit(
    const bool hit,
    const glm::vec3& position,
    const glm::vec3& normal)
    : hit_(hit),
      position_(position),
      normal_(normal)
{
}

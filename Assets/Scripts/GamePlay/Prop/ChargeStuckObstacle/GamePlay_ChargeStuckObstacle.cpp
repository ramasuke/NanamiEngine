#include "GamePlay_ChargeStuckObstacle.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    std::shared_ptr<ChargeStuckObstacle> ChargeStuckObstacle::FindFrom(GameObject::IGameObject& hitObject)
    {
        if (auto obstacle = hitObject.Components().Catch<ChargeStuckObstacle>().lock())
            return obstacle;

        auto current = hitObject.Transform().GetParent();
        while (current)
        {
            if (auto obstacle = current->Components().Catch<ChargeStuckObstacle>().lock())
                return obstacle;

            current = current->Transform().GetParent();
        }
        return nullptr;
    }
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::ChargeStuckObstacle, 0)

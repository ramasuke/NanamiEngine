#include "Physics_UserData.h"

#include "../../GameObject/Interface/IGameObject.h"

namespace NanamiEngine::Module::Physics
{
    UserData::UserData(
        const std::weak_ptr<GameObject::IGameObject>& entity)
        : entity_(entity)
    {
        
    }

    bool UserData::IsExpired()
    {
        return entity_.expired();
    }

    GameObject::ComponentGroup& UserData::Components() const
    {
        return entity_.lock()->Components();
    }
}

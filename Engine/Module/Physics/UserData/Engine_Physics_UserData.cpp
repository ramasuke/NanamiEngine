#include "Engine_Physics_UserData.h"

#include "../../GameObject/Interface/IGameObject.h"
#include "Jolt/Core/Core.h"

namespace NanamiEngine::Module::Physics
{
    UserData::UserData(
        const std::weak_ptr<GameObject::IGameObject>& entity)
        : entity_(entity)
    {
        
    }

    bool UserData::IsExpired() const
    {
        return entity_.expired();
    }

    GameObject::ComponentGroup& UserData::Components() const
    {
        return entity_.lock()->Components();
    }

    UserData* ToUserData(const JPH::uint64 userData)
    {
        return reinterpret_cast<UserData*>(userData);
    }
}

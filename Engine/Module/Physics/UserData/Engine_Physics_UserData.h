#pragma once
#include <memory>

namespace NanamiEngine::Module::GameObject
{
    class ComponentGroup;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Physics
{
    struct UserData final
    {
        explicit UserData(const std::weak_ptr<GameObject::IGameObject>& entity);

        [[nodiscard]] bool IsExpired() const;
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> Entity() const { return entity_; } 
        [[nodiscard]] GameObject::ComponentGroup& Components() const;

    private:
        std::weak_ptr<GameObject::IGameObject> entity_;
    };
    
    UserData* ToUserData(uint64_t userData);
}

#pragma once
namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace NanamiEngine::Core::Object
{
    class IInitializablePrefabObjectField
    {
    public:
        virtual ~IInitializablePrefabObjectField() = default;
        virtual void Init(Module::GameObject::Transform& root) = 0;
    };
}

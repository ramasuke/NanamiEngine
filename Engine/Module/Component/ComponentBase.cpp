#include "ComponentBase.h"

#include "../../Core/Object/Field/IInitializablePrefabField.h"

Component::ComponentBase:: ComponentBase() = default;
Component::ComponentBase::~ComponentBase() = default;

void Component::ComponentBase::InitComponent(
    const std::weak_ptr<GameObject::IGameObject>& ownerGameObject)
{
    gameObjectRef_ = ownerGameObject;
    for (const auto& prefabObjectField : prefabObjectFields_)
    {
        prefabObjectField->Init(Transform());
    }
}

void Component::ComponentBase::OnDrawGui()
{
    
}

void Component::ComponentBase::ResetGuid()
{
    guid_ = Guid();
}

void Component::ComponentBase::SetEnable(const bool enable)
{
    isEnable_ = enable;
}

bool Component::ComponentBase::IsEnable() const
{
    return isEnable_;
}

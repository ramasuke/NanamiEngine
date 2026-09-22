#include "ComponentBase.h"

Component::ComponentBase:: ComponentBase() = default;

Component::ComponentBase::~ComponentBase()
{
    // ComponentGroup::OnDestroy を通らずに捨てられた場合の保険
    ImplementCancelOnDestroy();
}

void Component::ComponentBase::InitComponent(
    const std::weak_ptr<GameObject::IGameObject>& ownerGameObject)
{
    gameObjectRef_ = ownerGameObject;
}

void Component::ComponentBase::OnDrawGui()
{
    
}

void Component::ComponentBase::ImplementCancelOnDestroy()
{
    destroyCancellationTokenSource_.Cancel();
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

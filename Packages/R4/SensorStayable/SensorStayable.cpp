#include "SensorStayable.h"

rxcpp::observable<std::pair<JPH::ContactManifold, std::shared_ptr<GameObject::IGameObject>>> R4::SensorStayable::OnAction() const
{
    return onAction_.get_observable();
}

void R4::SensorStayable::OnTriggerStay(const JPH::ContactManifold& contactManifold,
    const std::shared_ptr<GameObject::IGameObject>& gameObject)
{
    onAction_.get_subscriber().on_next(std::pair<const JPH::ContactManifold&, const std::shared_ptr<GameObject::IGameObject>&>(contactManifold, gameObject));
}

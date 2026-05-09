#include "SensorEnterable.h"

void R4::SensorEnterableAsObservable::OnTriggerEnter(
    const Physics::Manifold& contactManifold,
    const std::shared_ptr<GameObject::IGameObject>& gameObject)
{
    onAction_.get_subscriber().on_next(std::pair<const Physics::Manifold&, const std::shared_ptr<GameObject::IGameObject>&>(contactManifold, gameObject));
}

rxcpp::observable<std::pair<Physics::Manifold, std::shared_ptr<GameObject::IGameObject>>> R4::SensorEnterableAsObservable::OnAction() const
{
    return onAction_.get_observable();
}

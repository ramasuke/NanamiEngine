#include "SensorEnterableAsObservable.h"

void R4::SensorEnterableAsObservable::OnTriggerEnter(
    const Physics::Manifold& contactManifold,
    const std::shared_ptr<GameObject::IGameObject>& gameObject)
{
    onAction_.get_subscriber().on_next(SensorEnterContext(contactManifold, gameObject));
}

rxcpp::observable<R4::SensorEnterContext> R4::SensorEnterableAsObservable::OnAction() const
{
    return onAction_.get_observable();
}

#include "SensorEnterableAsObservable.h"

void R4::SensorEnterableAsObservable::OnTriggerEnter(
    const Physics::Manifold& contactManifold,
    const std::shared_ptr<GameObject::IGameObject>& gameObject)
{
    onAction_.OnNext(SensorEnterContext(contactManifold, gameObject));
}

R4::Observable<R4::SensorEnterContext> R4::SensorEnterableAsObservable::OnAction() const
{
    return onAction_.AsObservable();
}

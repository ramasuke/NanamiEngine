#include "SensorExitable.h"

rxcpp::observable<std::shared_ptr<GameObject::IGameObject>> R4::
SensorExitableAsObservable::OnAction() const
{
    return onAction_.get_observable();
}

void R4::SensorExitableAsObservable::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
{
    onAction_.get_subscriber().on_next(gameObject);
}

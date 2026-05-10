#include "SensorStayableAsObservable.h"

namespace NanamiEngine::R4
{
    rxcpp::observable<SensorStayContext> SensorStayableAsObservable::OnAction() const
    {
        return onAction_.get_observable();
    }

    void SensorStayableAsObservable::OnTriggerStay(
        const Physics::Manifold& contactManifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        onAction_.get_subscriber().on_next(SensorStayContext{
            contactManifold,
            gameObject
        });
    }

    void SensorStayableAsObservable::OnDrawGui()
    {
    }
}
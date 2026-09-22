#include "SensorStayableAsObservable.h"

namespace NanamiEngine::R4
{
    R4::Observable<SensorStayContext> SensorStayableAsObservable::OnAction() const
    {
        return onAction_.AsObservable();
    }

    void SensorStayableAsObservable::OnTriggerStay(
        const Physics::Manifold& contactManifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        onAction_.OnNext(SensorStayContext{
            contactManifold,
            gameObject
        });
    }

    void SensorStayableAsObservable::OnDrawGui()
    {
    }
}
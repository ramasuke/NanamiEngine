#include "SensorStayableAsObservable.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::R4::SensorStayableAsObservable);
#pragma endregion

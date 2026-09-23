#include "SensorEnterableAsObservable.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::R4::SensorEnterableAsObservable);
#pragma endregion

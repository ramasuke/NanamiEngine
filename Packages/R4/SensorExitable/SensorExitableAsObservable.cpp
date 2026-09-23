#include "SensorExitableAsObservable.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

R4::Observable<std::shared_ptr<GameObject::IGameObject>> R4::
SensorExitableAsObservable::OnAction() const
{
    return onAction_.AsObservable();
}

void R4::SensorExitableAsObservable::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
{
    onAction_.OnNext(gameObject);
}

void R4::SensorExitableAsObservable::OnDrawGui()
{
    
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::R4::SensorExitableAsObservable);
#pragma endregion

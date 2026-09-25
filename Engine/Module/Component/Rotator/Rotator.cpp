#include "Rotator.h"

#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

void Component::Rotator::OnUpdate()
{
    if (!IsEnable())
        return;

    const float angleRad = glm::radians(rotateSpeedDegPerSec_) * Time::DeltaTime();
    Transform().Rotate(glm::angleAxis(angleRad, glm::normalize(rotateAxis_)));
}

void Component::Rotator::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("rotateAxis_", rotateAxis_);
    ImGuiHelper::OnDrawInputField("rotateSpeedDegPerSec_", rotateSpeedDegPerSec_);
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::Rotator);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, NanamiEngine::Module::Component::Rotator);
#pragma endregion

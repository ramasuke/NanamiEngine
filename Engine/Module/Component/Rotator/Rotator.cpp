#include "Rotator.h"

#include "../../../Core/Application/Time/Time.h"
#include "../../GameObject/Transform/Transform.h"

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

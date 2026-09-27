#include "ChatIconChattableMotion.h"

#include <cmath>

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void ChatIconChattableMotion::OnUpdate()
    {
        if (!pop_.Update(*this, popDuration_secs_))
            return;

        glm::vec3 offset = {};
        offset.y = -std::abs(std::sin(pop_.ShownTime() * bounceSpeed_)) * bounceAmplitude_;
        pop_.Apply(*this, offset, pop_.PopScale(), pop_.BaseAngle());
    }

    void ChatIconChattableMotion::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("popDuration_secs_", popDuration_secs_);
        ImGuiHelper::OnDrawInputField("bounceAmplitude_", bounceAmplitude_);
        ImGuiHelper::OnDrawInputField("bounceSpeed_", bounceSpeed_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ChatIconChattableMotion);
#pragma endregion

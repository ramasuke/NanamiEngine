#include "ChatIconChattingMotion.h"

#include <cmath>

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        constexpr float PI = 3.14159265f;
    }

    void ChatIconChattingMotion::OnUpdate()
    {
        if (!pop_.Update(*this, popDuration_secs_))
            return;

        const float breath = 0.5f - 0.5f * std::cos(pop_.ShownTime() * 2.0f * PI / breathPeriod_secs_);
        pop_.Apply(*this, {}, pop_.PopScale() * (1.0f + breath * breathScale_), pop_.BaseAngle());
    }

    void ChatIconChattingMotion::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("popDuration_secs_", popDuration_secs_);
        ImGuiHelper::OnDrawInputField("breathScale_", breathScale_);
        ImGuiHelper::OnDrawInputField("breathPeriod_secs_", breathPeriod_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ChatIconChattingMotion);
#pragma endregion

#include "LayoutElement.h"
#include <algorithm>
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::NanamiUi
{
    void LayoutElement::SetMainAxisRate(const float mainAxisRate)
    {
        mainAxisRate_ = std::clamp(mainAxisRate, 0.0f, 1.0f);
    }

    float LayoutElement::MainAxisRate() const
    {
        return mainAxisRate_;
    }

    void LayoutElement::OnDrawGui()
    {
        ImGui::SliderFloat("mainAxisRate_", &mainAxisRate_, 0.0f, 1.0f);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::LayoutElement);
#pragma endregion

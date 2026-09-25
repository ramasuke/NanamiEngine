#include "DirectionLight.h"

#include "DxLib.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Component
{
    void DirectionLight::InitRenderer()
    {
        SetUseLighting(TRUE);
        SetLightEnable(TRUE);

        const VECTOR lightDir = VGet(-0.5f, -1.0f, -0.5f);
        SetLightDirection(lightDir);

        constexpr COLOR_F difColor = {1.0f, 1.0f, 1.0f, 1.0f};
        SetLightDifColor(difColor);
    }

    void NanamiEngine::Module::Component::DirectionLight::OnRender()
    {
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::DirectionLight);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IRenderable, NanamiEngine::Module::Component::DirectionLight);
#pragma endregion

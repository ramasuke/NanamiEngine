#include "UiFlow_DeviceHint.h"

#include "../../../Engine/Module/NanamiUI/NanamiUi_IInteractivableRenderer.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::UiFlow
{
    void DeviceHint::OnStart()
    {
        renderer_ = Components().Catch<Module::NanamiUi::IInteractivableRenderer>();
        Apply(InputDevice::Current());
    }

    void DeviceHint::OnUpdate()
    {
        if (!IsEnable())
            return;

        const auto device = InputDevice::Current();
        if (device != applied_)
            Apply(device);
    }

    void DeviceHint::Apply(const InputDeviceKind device)
    {
        applied_ = device;

        const auto& sprite = device == InputDeviceKind::Gamepad ? gamepadSprite_ : keyboardSprite_;
        const auto renderer = renderer_.lock();
        if (!renderer || !sprite)
            return;

        renderer->SetSprite(sprite.get());
    }

    void DeviceHint::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("keyboardSprite_", keyboardSprite_);
        ImGuiHelper::OnDrawInputField("gamepadSprite_", gamepadSprite_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::UiFlow::DeviceHint);
#pragma endregion

#include "PlayerAvatarInputActionBase.h"
#include "../Input/PlayerAvatarInput_void.h"

#include "../RequireType/RequireType.h"

namespace
{
    constexpr short XINPUT_THUMB_DEAD_ZONE = 8000;
    constexpr unsigned char XINPUT_TRIGGER_DEAD_ZONE = 30;
}

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::OnUpdate()
{
    gamepad_ = NanamiEngine::Platform::Input::Gamepad::Get();

    UpdateMouseWheel();
    UpdateCurrentDevice();

    for (const auto& input : inputs_)
    {
        input->OnUpdate();
    }
}

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::UpdateMouseWheel()
{
    const int mouseWheel = NanamiEngine::Platform::Input::Mouse::WheelRotation(false);
    mouseWheelDelta_    = mouseWheel - previousMouseWheel_;
    previousMouseWheel_ = mouseWheel;
}

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::UpdateCurrentDevice()
{
    const bool isGamepadTouched = gamepad_.IsAnyDown(XINPUT_TRIGGER_DEAD_ZONE, XINPUT_THUMB_DEAD_ZONE);

    const glm::ivec2 mouse = NanamiEngine::Platform::Input::Mouse::Position();
    const bool isMouseMoved = mouse.x != previousMouseX_ || mouse.y != previousMouseY_;
    previousMouseX_ = mouse.x;
    previousMouseY_ = mouse.y;

    // NOTE: IsAnyDeviceDown はパッドのボタンも含む (元の CheckHitKeyAll() と同じ)
    const bool isKeyboardTouched = NanamiEngine::Platform::Input::IsAnyDeviceDown() || NanamiEngine::Platform::Input::Mouse::Buttons() != 0 || isMouseMoved || mouseWheelDelta_ != 0;

    // 両方同時なら直前の機器のまま、どちらも無ければ維持
    if (isGamepadTouched && !isKeyboardTouched)
        currentDevice_ = PlayerAvatarInputDevice::Gamepad;
    else if (isKeyboardTouched && !isGamepadTouched)
        currentDevice_ = PlayerAvatarInputDevice::KeyboardMouse;
}

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::Enable()
{
    for (const auto& input : inputs_)
        input->Enable();
}

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::Disable()
{
    for (const auto& input : inputs_)
        input->Disable();
}

GameCore::PlayerAvatar::PlayerAvatarInputActionBase::Input<void> GameCore::PlayerAvatar::
PlayerAvatarInputActionBase::MakeInputAction(const std::function<bool()>& checkInput)
{
    auto input = std::make_shared<PlayerAvatarInput<void>>(checkInput);
    inputs_.push_back(input);
    return input;
}

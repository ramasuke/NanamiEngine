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
    GetJoypadXInputState(DX_INPUT_PAD1, &xInput_ ) ;

    UpdateMouseWheel();
    UpdateCurrentDevice();

    for (const auto& input : inputs_)
    {
        input->OnUpdate();
    }
}

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::UpdateMouseWheel()
{
    const int mouseWheel = GetMouseWheelRotVol(FALSE);
    mouseWheelDelta_    = mouseWheel - previousMouseWheel_;
    previousMouseWheel_ = mouseWheel;
}

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::UpdateCurrentDevice()
{
    bool isGamepadTouched = xInput_.LeftTrigger > XINPUT_TRIGGER_DEAD_ZONE || xInput_.RightTrigger > XINPUT_TRIGGER_DEAD_ZONE;
    for (const auto button : xInput_.Buttons)
        isGamepadTouched |= button != 0;
    for (const auto thumb : { xInput_.ThumbLX, xInput_.ThumbLY, xInput_.ThumbRX, xInput_.ThumbRY })
        isGamepadTouched |= thumb > XINPUT_THUMB_DEAD_ZONE || thumb < -XINPUT_THUMB_DEAD_ZONE;

    int mouseX = previousMouseX_;
    int mouseY = previousMouseY_;
    GetMousePoint(&mouseX, &mouseY);
    const bool isMouseMoved = mouseX != previousMouseX_ || mouseY != previousMouseY_;
    previousMouseX_ = mouseX;
    previousMouseY_ = mouseY;

    const bool isKeyboardTouched = CheckHitKeyAll() != 0 || GetMouseInput() != 0 || isMouseMoved || mouseWheelDelta_ != 0;

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

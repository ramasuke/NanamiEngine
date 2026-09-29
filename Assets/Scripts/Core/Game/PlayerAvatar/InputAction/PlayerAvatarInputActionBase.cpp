#include "PlayerAvatarInputActionBase.h"
#include "../Input/PlayerAvatarInput_void.h"

#include "../RequireType/RequireType.h"

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::OnUpdate()
{
    gamepad_ = NanamiEngine::Platform::Input::Gamepad::Get();

    UpdateMouseWheel();

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

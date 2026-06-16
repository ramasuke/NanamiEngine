#include "PlayerAvatarInputActionBase.h"
#include "../Input/PlayerAvatarInput_void.h"

#include "../RequireType/RequireType.h"

void GameCore::PlayerAvatar::PlayerAvatarInputActionBase::OnUpdate()
{
    GetJoypadXInputState(DX_INPUT_PAD1, &xInput_ ) ;
    
    for (const auto& input : inputs_)
    {
        input->OnUpdate();
    }
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

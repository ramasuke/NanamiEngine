#include "InputMove.h"
#include "DxLib.h"
#include "detail/func_geometric.inl"

namespace
{
    constexpr float STICK_MAX = 32767.0f;
    constexpr int DEAD_ZONE = 8000;
}


GameCore::PlayerAvatar::Input::InputMove::InputMove()
    : PlayerAvatarInput(IsPressed, GetMoveDirection)
{
}

bool GameCore::PlayerAvatar::Input::InputMove::IsPressed()
{
    if (IsPressedForKeyBoard() || IsPressedForXboxController())
        return true;
    
    return false;
}

bool GameCore::PlayerAvatar::Input::InputMove::IsPressedForKeyBoard()
{
    return CheckHitKey(KEY_INPUT_D) ||
           CheckHitKey(KEY_INPUT_W) ||
           CheckHitKey(KEY_INPUT_A) ||
           CheckHitKey(KEY_INPUT_S);
}

bool GameCore::PlayerAvatar::Input::InputMove::IsPressedForXboxController()
{
    DxLib::XINPUT_STATE input{};
    if (GetJoypadXInputState(DX_INPUT_PAD1, &input) == 0)
        return std::abs(input.ThumbLX) > DEAD_ZONE || std::abs(input.ThumbLY) > DEAD_ZONE;

    
    return false;
}

glm::vec2 GameCore::PlayerAvatar::Input::InputMove::GetMoveDirection()
{
    glm::vec2 moveInput(0.0f, 0.0f);
    moveInput += GetMoveDirectionForKeyBoard();
    moveInput += GetMoveDirectionForXboxController();

    if (moveInput == glm::vec2(0.0f))
        return moveInput;

    return glm::normalize(moveInput);
}

glm::vec2 GameCore::PlayerAvatar::Input::InputMove::GetMoveDirectionForKeyBoard()
{
    glm::vec2 moveInput(0.0f, 0.0f);
    if (CheckHitKey(KEY_INPUT_D))
        moveInput.x += 1.0f;
    if (CheckHitKey(KEY_INPUT_A))
        moveInput.x -= 1.0f;
    if (CheckHitKey(KEY_INPUT_W))
        moveInput.y -= 1.0f;
    if (CheckHitKey(KEY_INPUT_S))
        moveInput.y += 1.0f;
    
    return moveInput;
}

glm::vec2 GameCore::PlayerAvatar::Input::InputMove::GetMoveDirectionForXboxController()
{
    glm::vec2 moveInput(0.0f, 0.0f);
    DxLib::XINPUT_STATE input{};
    if (GetJoypadXInputState(DX_INPUT_PAD1, &input) == 0)
    {
        if (std::abs(input.ThumbLX) > DEAD_ZONE)
        {
            moveInput.x += static_cast<float>(input.ThumbLX) / STICK_MAX;
        }

        if (std::abs(input.ThumbLY) > DEAD_ZONE)
        {
            moveInput.y += -static_cast<float>(input.ThumbLY) / STICK_MAX;
        }
    }
    return moveInput;
}

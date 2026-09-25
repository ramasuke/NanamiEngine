#include "InputMove.h"
#include "Engine/Core/Platform/Input/Input.h"
#include "detail/func_geometric.inl"

namespace PlatformInput = NanamiEngine::Platform::Input;

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
    return PlatformInput::Keyboard::IsDown(PlatformInput::Key::D) ||
           PlatformInput::Keyboard::IsDown(PlatformInput::Key::W) ||
           PlatformInput::Keyboard::IsDown(PlatformInput::Key::A) ||
           PlatformInput::Keyboard::IsDown(PlatformInput::Key::S);
}

bool GameCore::PlayerAvatar::Input::InputMove::IsPressedForXboxController()
{
    const auto input = PlatformInput::Gamepad::Get();
    if (input.connected)
        return std::abs(input.thumbLX) > DEAD_ZONE || std::abs(input.thumbLY) > DEAD_ZONE;

    
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
    if (PlatformInput::Keyboard::IsDown(PlatformInput::Key::D))
        moveInput.x += 1.0f;
    if (PlatformInput::Keyboard::IsDown(PlatformInput::Key::A))
        moveInput.x -= 1.0f;
    if (PlatformInput::Keyboard::IsDown(PlatformInput::Key::W))
        moveInput.y -= 1.0f;
    if (PlatformInput::Keyboard::IsDown(PlatformInput::Key::S))
        moveInput.y += 1.0f;
    
    return moveInput;
}

glm::vec2 GameCore::PlayerAvatar::Input::InputMove::GetMoveDirectionForXboxController()
{
    glm::vec2 moveInput(0.0f, 0.0f);
    const auto input = PlatformInput::Gamepad::Get();
    if (input.connected)
    {
        if (std::abs(input.thumbLX) > DEAD_ZONE)
        {
            moveInput.x += static_cast<float>(input.thumbLX) / STICK_MAX;
        }

        if (std::abs(input.thumbLY) > DEAD_ZONE)
        {
            moveInput.y += -static_cast<float>(input.thumbLY) / STICK_MAX;
        }
    }
    return moveInput;
}

#pragma once
#include "vec2.hpp"
#include "../PlayerAvatarInput.h"

namespace GameCore::PlayerAvatar::Input
{
    class InputMove final : public PlayerAvatarInput<glm::vec2>
    {
    public:
        explicit InputMove();
        
    private:
        static bool IsPressed();
        static bool IsPressedForKeyBoard();
        static bool IsPressedForXboxController();
        static glm::vec2 GetMoveDirection();
        static glm::vec2 GetMoveDirectionForKeyBoard();
        static glm::vec2 GetMoveDirectionForXboxController();
    };
}

#pragma once
#include <memory>

#include "DxLib.h"
#include "../../Input/Move/InputMove.h"
#include "../../InputAction/PlayerAvatarInputActionBase.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatarInputAction final : public PlayerAvatarInputActionBase
    {
    public:
        [[nodiscard]] InputRef<glm::vec2>& Move() const { return *move_; }
        [[nodiscard]] InputRef<void     >& Run () const { return *run_ ; }
        [[nodiscard]] InputRef<void     >& Jump() const { return *jump_; }
        [[nodiscard]] InputRef<void     >& Cast() const { return *cast_; }

        void OnDrawGui() override;

    private:
        Input<glm::vec2> move_ = MakeInputAction<PlayerAvatar::Input::InputMove>();
        Input<void     > run_  = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_LSHIFT) || XInput().Buttons[XINPUT_BUTTON_A]; });
        Input<void     > jump_ = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_SPACE ) || XInput().Buttons[XINPUT_BUTTON_B]; });
        Input<void     > cast_ = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT || XInput().RightTrigger; });
    };
}

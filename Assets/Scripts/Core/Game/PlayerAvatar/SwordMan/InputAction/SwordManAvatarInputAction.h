#pragma once
#include <memory>

#include "DxLib.h"
#include "../../Input/Move/InputMove.h"
#include "../../InputAction/PlayerAvatarInputActionBase.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarInputAction final : public PlayerAvatarInputActionBase
    {
    public:
        [[nodiscard]] InputRef<glm::vec2>& Move        () const { return *move_        ; }
        [[nodiscard]] InputRef<void     >& Run         () const { return *run_         ; }
        [[nodiscard]] InputRef<void     >& Jump        () const { return *jump_        ; }
        [[nodiscard]] InputRef<void     >& NormalAttack() const { return *normalAttack_; }
        [[nodiscard]] InputRef<void     >& DashAttack  () const { return *dashAttack_  ; }
        [[nodiscard]] InputRef<void     >& CannonAttack() const { return *cannonAttack_; }
        [[nodiscard]] InputRef<void     >& Chat        () const { return *chat_        ; }
        [[nodiscard]] InputRef<void     >& OnReinforce () const { return *onReinforce_ ; }
        [[nodiscard]] InputRef<void     >& AvoidRolling() const { return *avoidRolling_; }
        
        void OnDrawGui() override;

    private:
        Input<glm::vec2> move_         = MakeInputAction<PlayerAvatar::Input::InputMove>();
        Input<void     > run_          = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_LSHIFT      ) || XInput().Buttons[XINPUT_BUTTON_A]; });
        Input<void     > jump_         = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_SPACE       ) || XInput().Buttons[XINPUT_BUTTON_B]; });
        Input<void     > normalAttack_ = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT  || XInput().RightTrigger; });
        Input<void     > dashAttack_   = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT  || XInput().RightTrigger; });
        Input<void     > cannonAttack_ = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT  || XInput().RightTrigger; });
        Input<void     > chat_         = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_E           ) || XInput().Buttons[XINPUT_BUTTON_Y]; });
        Input<void     > onReinforce_  = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_F           ) || XInput().LeftTrigger;  });
        Input<void     > avoidRolling_ = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_LCONTROL    ) || XInput().Buttons[XINPUT_BUTTON_X]; });
    };
}

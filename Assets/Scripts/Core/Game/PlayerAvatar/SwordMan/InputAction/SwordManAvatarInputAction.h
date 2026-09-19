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
        [[nodiscard]] InputRef<void     >& AvoidRolling() const { return *avoidRolling_; }
        [[nodiscard]] InputRef<void     >& LockOn      () const { return *lockOn_      ; }
        [[nodiscard]] InputRef<void     >& LockOnSwitchLeft () const { return *lockOnSwitchLeft_ ; }
        [[nodiscard]] InputRef<void     >& LockOnSwitchRight() const { return *lockOnSwitchRight_; }
        [[nodiscard]] InputRef<void     >& CycleItemNext() const { return *cycleItemNext_; }
        [[nodiscard]] InputRef<void     >& CycleItemPrev() const { return *cycleItemPrev_; }
        [[nodiscard]] InputRef<void     >& UseItem      () const { return *useItem_      ; }
        [[nodiscard]] InputRef<void     >& OpenMenu     () const { return *openMenu_     ; }

        void OnDrawGui() override;

    private:
        // 右スティックは弾いたと分かるくらい倒した時だけ切り替える
        static constexpr short LOCK_ON_SWITCH_STICK_THRESHOLD = 24000;

        Input<glm::vec2> move_         = MakeInputAction<PlayerAvatar::Input::InputMove>();
        Input<void     > run_          = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_LSHIFT      ) || XInput().Buttons[XINPUT_BUTTON_A]; });
        Input<void     > jump_         = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_SPACE       ) || XInput().Buttons[XINPUT_BUTTON_B]; });
        Input<void     > normalAttack_ = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT  || XInput().RightTrigger; });
        Input<void     > dashAttack_   = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT  || XInput().RightTrigger; });
        Input<void     > cannonAttack_ = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT  || XInput().RightTrigger; });
        Input<void     > chat_         = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_E           ) || XInput().Buttons[XINPUT_BUTTON_Y]; });
        Input<void     > avoidRolling_ = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_LCONTROL    ) || XInput().Buttons[XINPUT_BUTTON_X]; });
        Input<void     > lockOn_       = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_Q           ) || XInput().Buttons[XINPUT_BUTTON_RIGHT_THUMB]; });
        Input<void     > lockOnSwitchLeft_  = MakeInputAction([this] { return MouseWheelDelta() > 0 || XInput().ThumbRX < -LOCK_ON_SWITCH_STICK_THRESHOLD; });
        Input<void     > lockOnSwitchRight_ = MakeInputAction([this] { return MouseWheelDelta() < 0 || XInput().ThumbRX >  LOCK_ON_SWITCH_STICK_THRESHOLD; });
        Input<void     > cycleItemNext_= MakeInputAction([this] { return CheckHitKey(KEY_INPUT_X           ) || XInput().Buttons[XINPUT_BUTTON_DPAD_RIGHT]; });
        Input<void     > cycleItemPrev_= MakeInputAction([this] { return CheckHitKey(KEY_INPUT_Z           ) || XInput().Buttons[XINPUT_BUTTON_DPAD_LEFT ]; });
        Input<void     > useItem_      = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_R           ) || XInput().Buttons[XINPUT_BUTTON_LEFT_SHOULDER]; });
        Input<void     > openMenu_     = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_AT          ) || XInput().Buttons[XINPUT_BUTTON_START]; });
    };
}

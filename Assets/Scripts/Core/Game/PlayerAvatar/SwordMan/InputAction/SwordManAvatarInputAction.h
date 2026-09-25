#pragma once
#include <memory>

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
        Input<void     > run_          = MakeInputAction([this] { return IsKeyDown(Key::LShift) || IsPadDown(PadButton::A); });
        Input<void     > jump_         = MakeInputAction([this] { return IsKeyDown(Key::Space) || IsPadDown(PadButton::B); });
        Input<void     > normalAttack_ = MakeInputAction([this] { return IsMouseDown(MouseButton::Left)  || Gamepad().rightTrigger; });
        Input<void     > dashAttack_   = MakeInputAction([this] { return IsMouseDown(MouseButton::Left)  || Gamepad().rightTrigger; });
        Input<void     > cannonAttack_ = MakeInputAction([this] { return IsMouseDown(MouseButton::Left)  || Gamepad().rightTrigger; });
        Input<void     > chat_         = MakeInputAction([this] { return IsKeyDown(Key::E) || IsPadDown(PadButton::Y); });
        Input<void     > avoidRolling_ = MakeInputAction([this] { return IsKeyDown(Key::LControl) || IsPadDown(PadButton::X); });
        Input<void     > lockOn_       = MakeInputAction([this] { return IsKeyDown(Key::Q) || IsPadDown(PadButton::RightThumb); });
        Input<void     > lockOnSwitchLeft_  = MakeInputAction([this] { return MouseWheelDelta() > 0 || Gamepad().thumbRX < -LOCK_ON_SWITCH_STICK_THRESHOLD; });
        Input<void     > lockOnSwitchRight_ = MakeInputAction([this] { return MouseWheelDelta() < 0 || Gamepad().thumbRX >  LOCK_ON_SWITCH_STICK_THRESHOLD; });
        Input<void     > cycleItemNext_= MakeInputAction([this] { return IsKeyDown(Key::X) || IsPadDown(PadButton::DPadRight); });
        Input<void     > cycleItemPrev_= MakeInputAction([this] { return IsKeyDown(Key::Z) || IsPadDown(PadButton::DPadLeft); });
        Input<void     > useItem_      = MakeInputAction([this] { return IsKeyDown(Key::R) || IsPadDown(PadButton::LeftShoulder); });
        Input<void     > openMenu_     = MakeInputAction([this] { return IsKeyDown(Key::At) || IsPadDown(PadButton::Start); });
    };
}

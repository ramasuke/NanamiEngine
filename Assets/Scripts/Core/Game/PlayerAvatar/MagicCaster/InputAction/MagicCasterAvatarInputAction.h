#pragma once
#include <array>
#include <memory>
#include <optional>

#include "DxLib.h"
#include "../../Input/Move/InputMove.h"
#include "../../Input/PlayerAvatarInput_void.h"
#include "../../InputAction/PlayerAvatarInputActionBase.h"
#include "../Spell/MagicCasterSpellSlot.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    /**
     * LT を押している間だけ A/B/X/Y が魔法の枠になり、走る・ジャンプ・会話・回避は効かなくなる。
     * キーボードは 1〜4 を直接押し、右クリックを押しながらで2ページ目
     */
    class MagicCasterAvatarInputAction final : public PlayerAvatarInputActionBase
    {
    public:
        [[nodiscard]] InputRef<glm::vec2>& Move     () const { return *move_;      }
        [[nodiscard]] InputRef<void     >& Run      () const { return *run_;       }
        [[nodiscard]] InputRef<void     >& Jump     () const { return *jump_;      }
        [[nodiscard]] InputRef<void     >& Cast     () const { return *cast_;      }
        [[nodiscard]] InputRef<void     >& Chat     () const { return *chat_;      }
        [[nodiscard]] InputRef<void     >& AvoidRolling() const { return *avoidRolling_; }
        [[nodiscard]] InputRef<void     >& LockOn   () const { return *lockOn_;    }
        [[nodiscard]] InputRef<void     >& LockOnSwitchLeft () const { return *lockOnSwitchLeft_;  }
        [[nodiscard]] InputRef<void     >& LockOnSwitchRight() const { return *lockOnSwitchRight_; }
        [[nodiscard]] InputRef<void     >& Palette  () const { return *palette_;   }
        [[nodiscard]] InputRef<void     >& PageShift() const { return *pageShift_; }
        [[nodiscard]] InputRef<void     >& CycleItemNext() const { return *cycleItemNext_; }
        [[nodiscard]] InputRef<void     >& CycleItemPrev() const { return *cycleItemPrev_; }
        [[nodiscard]] InputRef<void     >& UseItem      () const { return *useItem_      ; }

        /** @brief この瞬間に押された持ち込み枠（0〜7）。押されていなければ nullopt */
        [[nodiscard]] std::optional<int> PressedLoadoutSlot() const;
        /** @brief 2ページ目（4〜7）を指しているか */
        [[nodiscard]] bool IsSecondPage() const { return pageShift_->IsUpdatePressed(); }

        void OnDrawGui() override;

    private:
        static constexpr unsigned char PALETTE_TRIGGER_DEAD_ZONE = 30;
        // 右スティックは弾いたと分かるくらい倒した時だけ切り替える
        static constexpr short LOCK_ON_SWITCH_STICK_THRESHOLD = 24000;

        [[nodiscard]] bool IsPaletteTriggerHeld() const { return XInput().LeftTrigger > PALETTE_TRIGGER_DEAD_ZONE; }
        [[nodiscard]] bool IsPadSlotPressed(const int button) const { return IsPaletteTriggerHeld() && XInput().Buttons[button]; }

        Input<glm::vec2> move_      = MakeInputAction<PlayerAvatar::Input::InputMove>();
        Input<void     > run_       = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_LSHIFT) || (!IsPaletteTriggerHeld() && XInput().Buttons[XINPUT_BUTTON_A]); });
        Input<void     > jump_      = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_SPACE ) || (!IsPaletteTriggerHeld() && XInput().Buttons[XINPUT_BUTTON_B]); });
        Input<void     > cast_      = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_LEFT || XInput().RightTrigger; });
        Input<void     > chat_      = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_E     ) || (!IsPaletteTriggerHeld() && XInput().Buttons[XINPUT_BUTTON_Y]); });
        // 剣士と同じ割り当て
        Input<void     > avoidRolling_ = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_LCONTROL) || (!IsPaletteTriggerHeld() && XInput().Buttons[XINPUT_BUTTON_X]); });
        Input<void     > lockOn_    = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_Q     ) || XInput().Buttons[XINPUT_BUTTON_RIGHT_THUMB]; });
        Input<void     > lockOnSwitchLeft_  = MakeInputAction([this] { return MouseWheelDelta() > 0 || XInput().ThumbRX < -LOCK_ON_SWITCH_STICK_THRESHOLD; });
        Input<void     > lockOnSwitchRight_ = MakeInputAction([this] { return MouseWheelDelta() < 0 || XInput().ThumbRX >  LOCK_ON_SWITCH_STICK_THRESHOLD; });
        Input<void     > palette_   = MakeInputAction([this] { return IsPaletteTriggerHeld(); });
        Input<void     > pageShift_ = MakeInputAction([this] { return GetMouseInput() & MOUSE_INPUT_RIGHT || XInput().Buttons[XINPUT_BUTTON_RIGHT_SHOULDER]; });
        // アイテムは剣士と同じ割り当て
        Input<void     > cycleItemNext_ = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_X) || XInput().Buttons[XINPUT_BUTTON_DPAD_RIGHT]; });
        Input<void     > cycleItemPrev_ = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_Z) || XInput().Buttons[XINPUT_BUTTON_DPAD_LEFT ]; });
        Input<void     > useItem_       = MakeInputAction([this] { return CheckHitKey(KEY_INPUT_R) || XInput().Buttons[XINPUT_BUTTON_LEFT_SHOULDER]; });

        // 時計回りに 上=Y(1) 右=B(2) 下=A(3) 左=X(4)
        std::array<Input<void>, SPELL_SLOTS_PER_PAGE> slots_ =
        {
            MakeInputAction([this] { return CheckHitKey(KEY_INPUT_1) || IsPadSlotPressed(XINPUT_BUTTON_Y); }),
            MakeInputAction([this] { return CheckHitKey(KEY_INPUT_2) || IsPadSlotPressed(XINPUT_BUTTON_B); }),
            MakeInputAction([this] { return CheckHitKey(KEY_INPUT_3) || IsPadSlotPressed(XINPUT_BUTTON_A); }),
            MakeInputAction([this] { return CheckHitKey(KEY_INPUT_4) || IsPadSlotPressed(XINPUT_BUTTON_X); }),
        };
    };
}

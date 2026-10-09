#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <array>
#include <cstdint>

#include "UiFlow_UiActionMap.h"

namespace NanamiEngine::UiFlow
{
    class UiScreen;

    // NOTE: メニュー操作の入力を読む。押した瞬間・押し続け・リピートを返す
    // NOTE: 問い合わせたときに 1 フレームに 1 回だけ読み直すので、Update を呼ぶ必要はない
    class NANAMI_API UiInputReader final
    {
    public:
        void SetMap(UiActionMap map);
        [[nodiscard]] UiActionMap& Map() { return map_; }
        void SetRepeat(float delay_secs, float interval_secs);
        // NOTE: いま押されている入力を、一度離されるまで無視する
        void WaitForRelease();

        [[nodiscard]] bool IsPressed (UiAction action);
        [[nodiscard]] bool IsHeld    (UiAction action);
        
        // NOTE: 押した瞬間と、押し続けて delay を過ぎてからは interval ごとに true
        [[nodiscard]] bool IsRepeated(UiAction action);

        // NOTE: キー・マウス・パッドのどれかを押した瞬間
        [[nodiscard]] bool IsAnyPressed();
        // NOTE: 押した瞬間の数字キー (0-9)。無ければ -1
        [[nodiscard]] int  PressedDigit();

    private:
        friend class UiScreen;

        struct NANAMI_NO_API ActionState
        {
            bool  isDown      = false;
            bool  isPressed   = false;
            bool  isRepeated  = false;
            bool  isIgnored   = true;
            float held_secs   = 0.0f;
            float repeat_secs = 0.0f;
        };

        // NOTE: screen が最前面で開いているときだけ入力を返すようにする
        void SetGate(const UiScreen* screen);
        void Poll();
        void Step(ActionState& state, bool isDown, float deltaTime) const;
        [[nodiscard]] const ActionState& State(UiAction action);

        static constexpr int DIGIT_COUNT = 10;

        UiActionMap map_ = UiActionMap::Default();
        std::array<ActionState, static_cast<std::size_t>(UiAction::Count)> states_{};
        ActionState anyState_{};
        std::array<ActionState, DIGIT_COUNT> digitStates_{};
        const UiScreen* gate_ = nullptr;
        float repeatDelay_secs_    = 0.35f;
        float repeatInterval_secs_ = 0.08f;
        std::uint64_t lastPolledFrame_ = 0;
        bool hasPolled_ = false;
    };
}

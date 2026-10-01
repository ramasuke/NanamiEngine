#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <array>
#include <cstdint>

#include "UiFlow_UiAction.h"

namespace NanamiEngine::UiFlow
{
    /** @brief 論理アクションと物理入力の対応。画面ごとの違いは Default() に足し引きして作る */
    class NANAMI_API UiActionMap final
    {
    public:
        static constexpr std::int16_t DEFAULT_STICK_THRESHOLD = 12000;

        /** @brief 標準の割り当て */
        [[nodiscard]] static UiActionMap Default();

        UiActionMap& Set      (UiAction action, UiBinding binding);
        UiActionMap& AddKey   (UiAction action, Platform::Input::Key key);
        UiActionMap& AddButton(UiAction action, Platform::Input::GamepadButton button);
        UiActionMap& AddStick (UiAction action, StickDirection stick);
        UiActionMap& Clear    (UiAction action);
        UiActionMap& SetStickThreshold(std::int16_t threshold);

        [[nodiscard]] bool IsDown(UiAction action, const Platform::Input::GamepadState& pad) const;

    private:
        std::array<UiBinding, static_cast<std::size_t>(UiAction::Count)> bindings_{};
        std::int16_t stickThreshold_ = DEFAULT_STICK_THRESHOLD;
    };
}

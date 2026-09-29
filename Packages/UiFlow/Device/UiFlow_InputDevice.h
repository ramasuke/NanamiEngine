#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>

namespace NanamiEngine::UiFlow
{
    enum class InputDeviceKind : std::uint8_t
    {
        KeyboardMouse,
        Gamepad,
    };

    /** @brief いま使われている入力機器。操作ガイドの表記の切り替えに使う */
    class NANAMI_API InputDevice final
    {
    public:
        /**
         * @brief 最後に触られた機器。両方同時に触られている間と、どちらも触られていない間は変わらない
         * @note  問い合わせたときに 1 フレームに 1 回だけ読み直す
         */
        [[nodiscard]] static InputDeviceKind Current();
    };
}

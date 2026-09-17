#pragma once
#include <optional>

#include "../../../../../../../../Libs/glm/vec2.hpp"
#include "../../State/Transition/SwordManControlGuideFocus.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    /// 操作ガイドに「次に押させたい操作」を指示する側（チュートリアル）
    class IControlGuideFocusRequest
    {
    public:
        virtual ~IControlGuideFocusRequest() = default;

        virtual void SetFocus(SwordManControlGuideFocus target) = 0;
        virtual void MarkCleared() = 0;
        virtual void ClearFocus () = 0;
        /** @brief ガイドが報告した強調中の行の画面座標。行が出ていなければ空 */
        [[nodiscard]] virtual const std::optional<glm::vec2>& FocusAnchor() const = 0;
    };
}

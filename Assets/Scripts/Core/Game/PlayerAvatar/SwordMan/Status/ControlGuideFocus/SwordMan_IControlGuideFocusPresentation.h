#pragma once
#include <optional>

#include "Libs/glm/vec2.hpp"
#include "../../State/Transition/SwordManControlGuideFocus.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    /// 指示を受けて描く側（操作ガイド）
    class IControlGuideFocusPresentation
    {
    public:
        virtual ~IControlGuideFocusPresentation() = default;

        [[nodiscard]] virtual const SwordManControlGuideFocusState& Current() const = 0;
        virtual void ReportFocusAnchor(const std::optional<glm::vec2>& anchor) = 0;
    };
}

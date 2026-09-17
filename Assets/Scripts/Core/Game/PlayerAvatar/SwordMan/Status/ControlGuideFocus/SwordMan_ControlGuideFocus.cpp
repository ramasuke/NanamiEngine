#include "SwordMan_ControlGuideFocus.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    void ControlGuideFocus::SetFocus(const SwordManControlGuideFocus target)
    {
        state_ = SwordManControlGuideFocusState{ target, false };
    }

    void ControlGuideFocus::MarkCleared()
    {
        state_.isCleared = true;
    }

    void ControlGuideFocus::ClearFocus()
    {
        state_ = SwordManControlGuideFocusState{};
        anchor_.reset();
    }
}

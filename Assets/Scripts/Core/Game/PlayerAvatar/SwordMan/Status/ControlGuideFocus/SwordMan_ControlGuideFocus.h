#pragma once
#include "SwordMan_IControlGuideFocusPresentation.h"
#include "SwordMan_IControlGuideFocusRequest.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    /// チュートリアルと操作ガイドをつなぐだけの受け渡し口。Status が持つ
    class ControlGuideFocus final : public IControlGuideFocusRequest,
                                    public IControlGuideFocusPresentation
    {
    public:
        void SetFocus(SwordManControlGuideFocus target) override;
        void MarkCleared() override;
        void ClearFocus () override;
        [[nodiscard]] const std::optional<glm::vec2>& FocusAnchor() const override { return anchor_; }

        [[nodiscard]] const SwordManControlGuideFocusState& Current() const override { return state_; }
        void ReportFocusAnchor(const std::optional<glm::vec2>& anchor) override { anchor_ = anchor; }

    private:
        SwordManControlGuideFocusState state_;
        std::optional<glm::vec2> anchor_;
    };
}

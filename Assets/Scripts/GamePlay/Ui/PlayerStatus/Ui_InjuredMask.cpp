#include "Ui_InjuredMask.h"

#include <cmath>
#include "../../../../../Engine/Core/Application/Time/Time.h"

namespace GamePlay::Ui
{
    void InjuredMaskUI::OnAwake()
    {
        blendRenderer_ = Components().Catch<NanamiUi::BlendImageRenderer>();
    }

    void InjuredMaskUI::OnUpdate()
    {
        if (!isActive_ || !blendRenderer_)
            return;

        constexpr float twoPi     = 6.28318530f;
        const float     t         = Time::CurrentTime();
        const float     normalized = (std::sin(twoPi * t / cycleDuration_secs_) + 1.0f) * 0.5f;
        blendRenderer_->SetBlendRate(minBlendRate_ + static_cast<int>((maxBlendRate_ - minBlendRate_) * normalized));
    }

    void InjuredMaskUI::SetActive(const bool isActive)
    {
        isActive_ = isActive;
        if (!isActive_ && blendRenderer_)
            blendRenderer_->SetBlendRate(0);
    }

    void InjuredMaskUI::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("minBlendRate_",       minBlendRate_);
        ImGuiHelper::OnDrawInputField("maxBlendRate_",       maxBlendRate_);
        ImGuiHelper::OnDrawInputField("cycleDuration_secs_", cycleDuration_secs_);
    }
}

#include "BlendAnmiationRenderer.h"
#include "../../../Core/Application/Time/Time.h"
#include <algorithm>

namespace NanamiEngine::Module::NanamiUi
{
    void BlendAnimationRenderer::SetAddBlendRate_secs(const int value)
    {
        addBlendRate_secs_ = value;
    }

    void BlendAnimationRenderer::OnAwake()
    {
        blendImageRenderer_ = RequireComponent<BlendImageRenderer>();
        currentBlendRate_   = blendImageRenderer_->GetBlendRate();
    }

    void BlendAnimationRenderer::OnUpdate()
    {
        if (!IsEnable())
            return;
        
        currentBlendRate_ += static_cast<float>(addBlendRate_secs_) * Time::DeltaTime();

        currentBlendRate_ = std::clamp(currentBlendRate_, 0.0f, 255.0f);
        blendImageRenderer_->SetBlendRate(static_cast<int>(currentBlendRate_));
    }

    void BlendAnimationRenderer::OnDrawGui()
    {
        if (ImGui::Button("ChangeEnable"))
        {
            Entity().lock()->SetEnable(!IsEnable());
        }
        ImGuiHelper::OnDrawInputField("currentBlendRate_" , currentBlendRate_);
        ImGuiHelper::OnDrawInputField("addBlendRate_secs_", addBlendRate_secs_);
    }
}

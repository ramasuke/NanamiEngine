#include "Ui_DamageFlash.h"

#include <algorithm>
#include "Engine/Core/Application/Time/Time.h"

namespace GamePlay::Ui
{
    std::vector<DamageFlashUI*> DamageFlashUI::instances_;

    void DamageFlashUI::Flash(const float intensity, const float duration)
    {
        if (duration <= 0.0f)
            return;

        trauma_   = std::clamp(trauma_ + intensity, 0.0f, 1.0f);
        duration_ = duration;
    }

    void DamageFlashUI::Flash()
    {
        Flash(defaultIntensity_, defaultDuration_);
    }

    void DamageFlashUI::FlashMainScreen(const float intensity, const float duration)
    {
        for (auto* instance : instances_)
            instance->Flash(intensity, duration);
    }

    void DamageFlashUI::FlashMainScreen()
    {
        for (auto* instance : instances_)
            instance->Flash();
    }

    void DamageFlashUI::OnAwake()
    {
        blendRenderer_ = Components().Catch<NanamiUi::BlendImageRenderer>();
        instances_.push_back(this);
    }

    void DamageFlashUI::OnDestroy()
    {
        std::erase(instances_, this);
    }

    void DamageFlashUI::OnUpdate()
    {
        if (trauma_ <= 0.0f || !blendRenderer_)
            return;

        trauma_ = (std::max)(0.0f, trauma_ - Time::DeltaTime() / duration_);
        blendRenderer_->SetBlendRate(static_cast<int>(trauma_ * static_cast<float>(maxBlendRate_)));
    }

    void DamageFlashUI::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("maxBlendRate_",     maxBlendRate_);
        ImGuiHelper::OnDrawInputField("defaultIntensity_", defaultIntensity_);
        ImGuiHelper::OnDrawInputField("defaultDuration_",  defaultDuration_);

        if (ImGui::Button("Test Flash"))
            Flash();
    }
}

#include "Ui_DamageFlash.h"

#include <algorithm>
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    std::vector<DamageFlashUI*> DamageFlashUI::instances_;

    void DamageFlashUI::Flash(const float intensity, const float duration)
    {
        if (duration <= 0.0f)
            return;

        // NOTE: 残りに足して 1/duration 毎秒で減らすので、区間長は残量に比例させる
        const float trauma = std::clamp(trauma_.Value() + intensity, 0.0f, 1.0f);
        trauma_.Play(tweeny::from(trauma).to(0.0f).during(LibCore::Tween::Ms(duration * trauma)));
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
        if (!trauma_.IsPlaying() || !blendRenderer_)
            return;

        trauma_.Tick(Time::DeltaTime());
        blendRenderer_->SetBlendRate(static_cast<int>(trauma_.Value() * static_cast<float>(maxBlendRate_)));
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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::DamageFlashUI);
#pragma endregion

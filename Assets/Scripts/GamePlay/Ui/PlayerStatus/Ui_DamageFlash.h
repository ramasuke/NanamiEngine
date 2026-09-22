#pragma once
#include <vector>
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"

namespace GamePlay::Ui
{
    class DamageFlashUI final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable,
                                public LifeCycleCallback::IUpdatable
    {
    public:
        void Flash(float intensity, float duration);
        void Flash();

        static void FlashMainScreen(float intensity, float duration);
        static void FlashMainScreen();

    private:
        void OnAwake  () override;
        void OnDestroy() override;
        void OnUpdate () override;

        static std::vector<DamageFlashUI*> instances_;

        float trauma_   = 0.0f;
        float duration_ = 0.25f;

        [[serialize(0)]] int   maxBlendRate_     = 150;
        [[serialize(0)]] float defaultIntensity_ = 0.5f;
        [[serialize(0)]] float defaultDuration_  = 0.25f;

        FIELD(NanamiUi::BlendImageRenderer) blendRenderer_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(maxBlendRate_));
            archive(CEREAL_NVP(defaultIntensity_));
            archive(CEREAL_NVP(defaultDuration_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(maxBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(defaultIntensity_));
            if (version >= 0) archive(CEREAL_NVP(defaultDuration_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::DamageFlashUI, 0)

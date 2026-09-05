#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "Ui_InjuredMask.h"

namespace GameCore::StatusParameter
{
    struct Health;
    struct Stamina;
}

namespace GamePlay::Ui
{
    class PlayerStatus final : public Component::ComponentBase,
                               public LifeCycleCallback::IAwakable
    {
    public:
        void OnAwake() override;
        void UpdateHealthBar(
            const GameCore::StatusParameter::Health& maxHealth,
            const GameCore::StatusParameter::Health& health   ) const;
        void OnDamageHealthBar() const;
        void UpdateStaminaBar(
            const GameCore::StatusParameter::Stamina& maxStamina,
            const GameCore::StatusParameter::Stamina& stamina) const;
        void OnIsInjured(bool isInjured) const;

    private:
        Coroutine::Task<void> OnDamagedHealth() const;

        [[serialize(6)]] std::string healthBarName_;
        FIELD(NanamiUi::Slider) healthBar_;
        [[serialize(2)]] float displayOnDamageHealthBarDuration_secs_ = 0.0f;
        [[serialize(2)]] FIELD(Asset::SpriteFile) onDamageHealthBarFrame_;
        [[serialize(6)]] std::string healthBarFrameName_;
        FIELD(Component::ImageRenderer) healthBarFrame_;

        [[serialize(8)]] std::string staminaBarName_;
        FIELD(NanamiUi::Slider) staminaBar_;
        [[serialize(8)]] std::string staminaBarFrameName_;
        FIELD(Component::ImageRenderer) staminaBarFrame_;

        [[serialize(7)]] std::string injuredUiObjectName_;
        FIELD(InjuredMaskUI) injuredUiMask_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(healthBarName_));
            archive(CEREAL_NVP(displayOnDamageHealthBarDuration_secs_));
            archive(CEREAL_NVP(onDamageHealthBarFrame_));
            archive(CEREAL_NVP(healthBarFrameName_));
            archive(CEREAL_NVP(staminaBarName_));
            archive(CEREAL_NVP(staminaBarFrameName_));
            archive(CEREAL_NVP(injuredUiObjectName_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 6) archive(CEREAL_NVP(healthBarName_));
            if (version >= 6) archive(CEREAL_NVP(displayOnDamageHealthBarDuration_secs_));
            if (version >= 6) archive(CEREAL_NVP(onDamageHealthBarFrame_));
            if (version >= 6) archive(CEREAL_NVP(healthBarFrameName_));
            if (version >= 8) archive(CEREAL_NVP(staminaBarName_));
            if (version >= 8) archive(CEREAL_NVP(staminaBarFrameName_));
            if (version >= 7) archive(CEREAL_NVP(injuredUiObjectName_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::PlayerStatus, 8)


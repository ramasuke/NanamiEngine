#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"

namespace GameCore::PlayerAvatar
{
    struct EnhancePower;
}

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace GamePlay::Ui
{
    class PlayerStatus final : public Component::ComponentBase
    {
    public:
        void UpdateHealthBar(
            const GameCore::StatusParameter::Health& maxHealth,
            const GameCore::StatusParameter::Health& health   ) const;
        void OnDamageHealthBar() const;
        void UpdateEnhancePowerStackBar(
            const GameCore::PlayerAvatar::EnhancePower& maxEnhancePower,
            const GameCore::PlayerAvatar::EnhancePower& enhancePower) const;
        void OnAddEnhancePowerStack() const;
        void OnIsEnableReinforceMode(bool enable) const;
        
    private:
        Coroutine::Task<void> OnDamagedHealth() const;
        Coroutine::Task<void> OnAddedEnhancePowerStack() const;
        
        [[serialize(0)]] FIELD(NanamiUi::Slider) healthBar_;
        [[serialize(2)]] FIELD(Component::ImageRenderer) healthBarFrame_;
        [[serialize(2)]] float displayOnDamageHealthBarDuration_secs_ = 0.0f;
        [[serialize(2)]] FIELD(Asset::SpriteFile) onDamageHealthBarFrame_;

        [[serialize(0)]] FIELD(NanamiUi::Slider) enhanceBar_;
        [[serialize(4)]] FIELD(Component::ImageRenderer) enhancePowerStackBarFrame_;
        [[serialize(4)]] float displayOnAddEnhancePowerStackBarDuration_secs_ = 0.0f;
        [[serialize(4)]] FIELD(Asset::SpriteFile) onAddEnhancePowerStackBarFrame_;
        [[serialize(5)]] FIELD(NanamiUi::BlendImageRenderer) onEnableReinforceMask_;
        

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(healthBar_));
            archive(CEREAL_NVP(enhanceBar_));
            archive(CEREAL_NVP(healthBarFrame_));
            archive(CEREAL_NVP(displayOnDamageHealthBarDuration_secs_));
            archive(CEREAL_NVP(onDamageHealthBarFrame_));
            archive(CEREAL_NVP(enhancePowerStackBarFrame_));
            archive(CEREAL_NVP(displayOnAddEnhancePowerStackBarDuration_secs_));
            archive(CEREAL_NVP(onAddEnhancePowerStackBarFrame_));
            archive(CEREAL_NVP(onEnableReinforceMask_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(healthBar_));
            if (version >= 0) archive(CEREAL_NVP(enhanceBar_));
            if (version >= 2) archive(CEREAL_NVP(healthBarFrame_));
            if (version >= 2) archive(CEREAL_NVP(displayOnDamageHealthBarDuration_secs_));
            if (version >= 2) archive(CEREAL_NVP(onDamageHealthBarFrame_));
            if (version >= 4) archive(CEREAL_NVP(enhancePowerStackBarFrame_));
            if (version >= 4) archive(CEREAL_NVP(displayOnAddEnhancePowerStackBarDuration_secs_));
            if (version >= 4) archive(CEREAL_NVP(onAddEnhancePowerStackBarFrame_));
            if (version >= 5) archive(CEREAL_NVP(onEnableReinforceMask_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Ui::PlayerStatus, 5);
CEREAL_REGISTER_TYPE(GamePlay::Ui::PlayerStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Ui::PlayerStatus);
#pragma endregion


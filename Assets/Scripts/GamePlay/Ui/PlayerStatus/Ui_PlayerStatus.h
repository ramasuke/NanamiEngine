#pragma once
#include "../../../../../Engine/Core/Coroutine/Task/Task.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Ui_InjuredMask.h"

namespace GameCore::StatusParameter
{
    struct Health;
    struct Stamina;
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
        void UpdateStaminaBar(
            const GameCore::StatusParameter::Stamina& maxStamina,
            const GameCore::StatusParameter::Stamina& stamina) const;
        void OnIsInjured(bool isInjured) const;

    private:
        Coroutine::Task<void> OnDamagedHealth() const;
        [[nodiscard]] Color32 SelectHealthTextColor(float healthRate) const;
        [[nodiscard]] std::shared_ptr<Asset::SpriteFile> SelectHealthGaugeSprite(float healthRate) const;

        [[serialize(10)]] FIELD(NanamiUi::Slider) healthBar_;
        // 被ダメ時に現在HPの数字を赤くする時間（onDamageHealthBarFrame_ があればフレーム差し替えにも使う）
        [[serialize(2)]] float displayOnDamageHealthBarDuration_secs_ = 0.0f;
        [[serialize(2)]] FIELD(Asset::SpriteFile) onDamageHealthBarFrame_;
        [[serialize(10)]] FIELD(Component::ImageRenderer) healthBarFrame_;

        [[serialize(10)]] FIELD(NanamiUi::Slider) staminaBar_;
        [[serialize(10)]] FIELD(Component::ImageRenderer) staminaBarFrame_;

        [[serialize(10)]] FIELD(InjuredMaskUI) injuredUiMask_;

        [[serialize(10)]] FIELD(NanamiUi::TextRenderer) hpCurrentText_;
        [[serialize(10)]] FIELD(NanamiUi::TextRenderer) hpMaxText_;
        // HP残量で HealthBar のゲージ画像を切り替える（未設定なら切り替えない）
        [[serialize(9)]] FIELD(Asset::SpriteFile) healthGaugeNormalSprite_;
        [[serialize(9)]] FIELD(Asset::SpriteFile) healthGaugeCautionSprite_;
        [[serialize(9)]] FIELD(Asset::SpriteFile) healthGaugeDangerSprite_;
        [[serialize(9)]] float cautionHealthRate_ = 0.5f;
        [[serialize(9)]] float dangerHealthRate_ = 0.25f;
        // 現在HPの数字の色（被ダメ中は危険色）
        [[serialize(9)]] Color32 healthTextNormalColor_  = Color32(255, 255, 255);
        [[serialize(9)]] Color32 healthTextCautionColor_ = Color32(255, 214, 90);
        [[serialize(9)]] Color32 healthTextDangerColor_  = Color32(255, 96, 80);

        mutable int damageFlashCount_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(healthBar_));
            archive(CEREAL_NVP(displayOnDamageHealthBarDuration_secs_));
            archive(CEREAL_NVP(onDamageHealthBarFrame_));
            archive(CEREAL_NVP(healthBarFrame_));
            archive(CEREAL_NVP(staminaBar_));
            archive(CEREAL_NVP(staminaBarFrame_));
            archive(CEREAL_NVP(injuredUiMask_));
            archive(CEREAL_NVP(hpCurrentText_));
            archive(CEREAL_NVP(hpMaxText_));
            archive(CEREAL_NVP(healthGaugeNormalSprite_));
            archive(CEREAL_NVP(healthGaugeCautionSprite_));
            archive(CEREAL_NVP(healthGaugeDangerSprite_));
            archive(CEREAL_NVP(cautionHealthRate_));
            archive(CEREAL_NVP(dangerHealthRate_));
            archive(CEREAL_NVP(healthTextNormalColor_));
            archive(CEREAL_NVP(healthTextCautionColor_));
            archive(CEREAL_NVP(healthTextDangerColor_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            // v10 で子オブジェクトの名前検索を FIELD に置き換えた。旧ファイルの名前は読み捨てる
            std::string healthBarName_, healthBarFrameName_, staminaBarName_, staminaBarFrameName_;
            std::string injuredUiObjectName_, hpCurrentTextName_, hpMaxTextName_;
            if (version >= 6 && version < 10) archive(CEREAL_NVP(healthBarName_));
            if (version >= 10) archive(CEREAL_NVP(healthBar_));
            if (version >= 6) archive(CEREAL_NVP(displayOnDamageHealthBarDuration_secs_));
            if (version >= 6) archive(CEREAL_NVP(onDamageHealthBarFrame_));
            if (version >= 6 && version < 10) archive(CEREAL_NVP(healthBarFrameName_));
            if (version >= 10) archive(CEREAL_NVP(healthBarFrame_));
            if (version >= 8 && version < 10) archive(CEREAL_NVP(staminaBarName_));
            if (version >= 10) archive(CEREAL_NVP(staminaBar_));
            if (version >= 8 && version < 10) archive(CEREAL_NVP(staminaBarFrameName_));
            if (version >= 10) archive(CEREAL_NVP(staminaBarFrame_));
            if (version >= 7 && version < 10) archive(CEREAL_NVP(injuredUiObjectName_));
            if (version >= 10) archive(CEREAL_NVP(injuredUiMask_));
            if (version >= 9 && version < 10) archive(CEREAL_NVP(hpCurrentTextName_));
            if (version >= 10) archive(CEREAL_NVP(hpCurrentText_));
            if (version >= 9 && version < 10) archive(CEREAL_NVP(hpMaxTextName_));
            if (version >= 10) archive(CEREAL_NVP(hpMaxText_));
            if (version >= 9) archive(CEREAL_NVP(healthGaugeNormalSprite_));
            if (version >= 9) archive(CEREAL_NVP(healthGaugeCautionSprite_));
            if (version >= 9) archive(CEREAL_NVP(healthGaugeDangerSprite_));
            if (version >= 9) archive(CEREAL_NVP(cautionHealthRate_));
            if (version >= 9) archive(CEREAL_NVP(dangerHealthRate_));
            if (version >= 9) archive(CEREAL_NVP(healthTextNormalColor_));
            if (version >= 9) archive(CEREAL_NVP(healthTextCautionColor_));
            if (version >= 9) archive(CEREAL_NVP(healthTextDangerColor_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::PlayerStatus, 10)


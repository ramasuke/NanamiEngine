#pragma once
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Font/Ttf/TtfFontFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    // 大砲の再装填ゲージ。鋼のリングにゲージを溜め、満タンになった瞬間と発射した時に演出を再生する。
    // 画像は tools/art/cannon_cooldown_gauge.py で生成し、配置の数値（gaugeRadius_ や各 Offset）もそのスクリプトの出力に合わせる
    class CannonCooldownGauge final : public Component::ComponentBase,
                                      public LifeCycleCallback::IUserInterfaceRenderable,
                                      public LifeCycleCallback::IUpdatable
    {
    public:
        void Show();
        void Hide();
        void SetCooldown(float remain_secs, float total_secs);
        void PlayShoot();

    private:
        struct Pose
        {
            float gaugePercent = 0.0f;
            bool isGold = false;
            bool isTipVisible = false;
            float flash = 0.0f;
            float scale = 1.0f;
            float halo = 0.0f;
            float shockwaveRate = -1.0f;
            float bombDim = 0.0f;
            float bombAngle = 0.0f;
            float bombScale = 1.0f;
            glm::vec2 bombOffset = glm::vec2(0.0f);
            float bombAlpha = 1.0f;
            float spark = 0.0f;
            bool isPromptLit = false;
            float promptFlash = 0.0f;
            int count = 0;
            float countScale = 1.0f;
            float countAlpha = 0.0f;
        };

        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[nodiscard]] Pose EvaluatePose() const;
        void DrawCenteredText(const std::string& utf8Text, const glm::vec2& centre, float scale, const Color32& color, float alpha) const;

        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] FIELD(Asset::SpriteFile) frameSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) fillTealSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) fillGoldSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) fillFlashSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) tipSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) haloSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) shockwaveSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) bombSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) sparkSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) emberSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) promptPillSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) promptPillGlowSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) promptMouseSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) promptMouseLitSprite_;
        [[serialize(0)]] FIELD(Asset::TtfFontFile) font_;

        [[serialize(0)]] float gaugeRadius_ = 67.0f;
        [[serialize(0)]] float shockwaveSpriteRadius_ = 100.0f;
        [[serialize(0)]] float emberSpriteRadius_ = 3.6f;
        [[serialize(0)]] glm::vec2 bombPivotOffset_ = glm::vec2(-2.0f, 30.0f);
        [[serialize(0)]] glm::vec2 bombPivotInSprite_ = glm::vec2(32.0f, 65.0f);
        [[serialize(0)]] glm::vec2 sparkOffsetFromBombPivot_ = glm::vec2(25.0f, -63.0f);
        [[serialize(0)]] glm::vec2 pillOffset_ = glm::vec2(-150.0f, 0.0f);
        [[serialize(0)]] glm::vec2 mouseOffset_ = glm::vec2(-210.0f, 0.0f);
        [[serialize(0)]] glm::vec2 promptTextOffset_ = glm::vec2(-138.0f, -1.0f);
        [[serialize(0)]] glm::vec2 countTextOffset_ = glm::vec2(0.0f, 2.0f);

        // シーンの文字列と同じ UTF-8 で持つ（/utf-8 無しの MSVC は日本語リテラルを CP932 に変換してしまうため）。"装填中" / "発射"
        [[serialize(0)]] std::string coolingPromptText_ = "\xE8\xA3\x85\xE5\xA1\xAB\xE4\xB8\xAD";
        [[serialize(0)]] std::string readyPromptText_ = "\xE7\x99\xBA\xE5\xB0\x84";
        [[serialize(0)]] float promptTextScale_ = 0.42f;
        [[serialize(0)]] float countTextScale_ = 0.97f;
        [[serialize(0)]] Color32 promptTextColor_ = Color32(170, 180, 186);
        [[serialize(0)]] Color32 promptReadyTextColor_ = Color32(255, 214, 130);
        [[serialize(0)]] Color32 countTextColor_ = Color32(255, 255, 255);

        [[serialize(0)]] float bombDimRate_ = 0.8f;
        [[serialize(0)]] float readyFlashDuration_secs_ = 0.3f;
        [[serialize(0)]] float readyPunchAmplitude_ = 0.2f;
        [[serialize(0)]] float readyPunchFrequency_ = 22.0f;
        [[serialize(0)]] float readyPunchDamping_ = 7.0f;
        [[serialize(0)]] float shockwaveDuration_secs_ = 0.5f;
        [[serialize(0)]] float shockwaveStartRadius_ = 80.0f;
        [[serialize(0)]] float shockwaveEndRadius_ = 142.0f;
        [[serialize(0)]] int emberCount_ = 8;
        [[serialize(0)]] float emberDistance_ = 46.0f;
        [[serialize(0)]] float sparkPopDuration_secs_ = 0.28f;
        [[serialize(0)]] float wobbleAngle_deg_ = 5.0f;
        [[serialize(0)]] float wobbleFrequency_hz_ = 2.0f;
        [[serialize(0)]] float haloPulseFrequency_hz_ = 1.4f;
        [[serialize(0)]] float drainDuration_secs_ = 0.15f;
        [[serialize(0)]] float launchDuration_secs_ = 0.28f;
        [[serialize(0)]] glm::vec2 launchOffset_ = glm::vec2(26.0f, -58.0f);
        [[serialize(0)]] float recoilAmplitude_ = 0.1f;
        [[serialize(1)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        [[serialize(2)]] float bombBrighten_secs_ = 0.2f;
        [[serialize(2)]] float wobbleSettle_secs_ = 0.35f;
        [[serialize(2)]] float countFadeOut_secs_ = 0.1f;
        [[serialize(2)]] float countPopIn_secs_ = 0.18f;
        [[serialize(2)]] float bombFadeIn_secs_ = 0.25f;
        [[serialize(2)]] float launchShrinkRate_ = 0.6f;
        [[serialize(2)]] float recoilFrequency_ = 26.0f;
        [[serialize(2)]] float recoilDamping_ = 9.0f;
        [[serialize(2)]] float emberStartAngle_deg_ = 20.0f;

        float remain_secs_ = 0.0f;
        float total_secs_ = 1.0f;
        int lastCount_ = 0;
        bool isReady_ = false;
        // 乗った瞬間の「最初から装填済み」では装填完了の音を鳴らさない
        bool hasCountedDown_ = false;
        float readyElapsed_secs_ = 0.0f;
        float shootElapsed_secs_ = 1000.0f;
        float time_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(frameSprite_));
            archive(CEREAL_NVP(fillTealSprite_));
            archive(CEREAL_NVP(fillGoldSprite_));
            archive(CEREAL_NVP(fillFlashSprite_));
            archive(CEREAL_NVP(tipSprite_));
            archive(CEREAL_NVP(haloSprite_));
            archive(CEREAL_NVP(shockwaveSprite_));
            archive(CEREAL_NVP(bombSprite_));
            archive(CEREAL_NVP(sparkSprite_));
            archive(CEREAL_NVP(emberSprite_));
            archive(CEREAL_NVP(promptPillSprite_));
            archive(CEREAL_NVP(promptPillGlowSprite_));
            archive(CEREAL_NVP(promptMouseSprite_));
            archive(CEREAL_NVP(promptMouseLitSprite_));
            archive(CEREAL_NVP(font_));
            archive(CEREAL_NVP(gaugeRadius_));
            archive(CEREAL_NVP(shockwaveSpriteRadius_));
            archive(CEREAL_NVP(emberSpriteRadius_));
            archive(CEREAL_NVP(bombPivotOffset_));
            archive(CEREAL_NVP(bombPivotInSprite_));
            archive(CEREAL_NVP(sparkOffsetFromBombPivot_));
            archive(CEREAL_NVP(pillOffset_));
            archive(CEREAL_NVP(mouseOffset_));
            archive(CEREAL_NVP(promptTextOffset_));
            archive(CEREAL_NVP(countTextOffset_));
            archive(CEREAL_NVP(coolingPromptText_));
            archive(CEREAL_NVP(readyPromptText_));
            archive(CEREAL_NVP(promptTextScale_));
            archive(CEREAL_NVP(countTextScale_));
            archive(CEREAL_NVP(promptTextColor_));
            archive(CEREAL_NVP(promptReadyTextColor_));
            archive(CEREAL_NVP(countTextColor_));
            archive(CEREAL_NVP(bombDimRate_));
            archive(CEREAL_NVP(readyFlashDuration_secs_));
            archive(CEREAL_NVP(readyPunchAmplitude_));
            archive(CEREAL_NVP(readyPunchFrequency_));
            archive(CEREAL_NVP(readyPunchDamping_));
            archive(CEREAL_NVP(shockwaveDuration_secs_));
            archive(CEREAL_NVP(shockwaveStartRadius_));
            archive(CEREAL_NVP(shockwaveEndRadius_));
            archive(CEREAL_NVP(emberCount_));
            archive(CEREAL_NVP(emberDistance_));
            archive(CEREAL_NVP(sparkPopDuration_secs_));
            archive(CEREAL_NVP(wobbleAngle_deg_));
            archive(CEREAL_NVP(wobbleFrequency_hz_));
            archive(CEREAL_NVP(haloPulseFrequency_hz_));
            archive(CEREAL_NVP(drainDuration_secs_));
            archive(CEREAL_NVP(launchDuration_secs_));
            archive(CEREAL_NVP(launchOffset_));
            archive(CEREAL_NVP(recoilAmplitude_));
            archive(CEREAL_NVP(uiSounds_));
            archive(CEREAL_NVP(bombBrighten_secs_));
            archive(CEREAL_NVP(wobbleSettle_secs_));
            archive(CEREAL_NVP(countFadeOut_secs_));
            archive(CEREAL_NVP(countPopIn_secs_));
            archive(CEREAL_NVP(bombFadeIn_secs_));
            archive(CEREAL_NVP(launchShrinkRate_));
            archive(CEREAL_NVP(recoilFrequency_));
            archive(CEREAL_NVP(recoilDamping_));
            archive(CEREAL_NVP(emberStartAngle_deg_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(frameSprite_));
            if (version >= 0) archive(CEREAL_NVP(fillTealSprite_));
            if (version >= 0) archive(CEREAL_NVP(fillGoldSprite_));
            if (version >= 0) archive(CEREAL_NVP(fillFlashSprite_));
            if (version >= 0) archive(CEREAL_NVP(tipSprite_));
            if (version >= 0) archive(CEREAL_NVP(haloSprite_));
            if (version >= 0) archive(CEREAL_NVP(shockwaveSprite_));
            if (version >= 0) archive(CEREAL_NVP(bombSprite_));
            if (version >= 0) archive(CEREAL_NVP(sparkSprite_));
            if (version >= 0) archive(CEREAL_NVP(emberSprite_));
            if (version >= 0) archive(CEREAL_NVP(promptPillSprite_));
            if (version >= 0) archive(CEREAL_NVP(promptPillGlowSprite_));
            if (version >= 0) archive(CEREAL_NVP(promptMouseSprite_));
            if (version >= 0) archive(CEREAL_NVP(promptMouseLitSprite_));
            if (version >= 0) archive(CEREAL_NVP(font_));
            if (version >= 0) archive(CEREAL_NVP(gaugeRadius_));
            if (version >= 0) archive(CEREAL_NVP(shockwaveSpriteRadius_));
            if (version >= 0) archive(CEREAL_NVP(emberSpriteRadius_));
            if (version >= 0) archive(CEREAL_NVP(bombPivotOffset_));
            if (version >= 0) archive(CEREAL_NVP(bombPivotInSprite_));
            if (version >= 0) archive(CEREAL_NVP(sparkOffsetFromBombPivot_));
            if (version >= 0) archive(CEREAL_NVP(pillOffset_));
            if (version >= 0) archive(CEREAL_NVP(mouseOffset_));
            if (version >= 0) archive(CEREAL_NVP(promptTextOffset_));
            if (version >= 0) archive(CEREAL_NVP(countTextOffset_));
            if (version >= 0) archive(CEREAL_NVP(coolingPromptText_));
            if (version >= 0) archive(CEREAL_NVP(readyPromptText_));
            if (version >= 0) archive(CEREAL_NVP(promptTextScale_));
            if (version >= 0) archive(CEREAL_NVP(countTextScale_));
            if (version >= 0) archive(CEREAL_NVP(promptTextColor_));
            if (version >= 0) archive(CEREAL_NVP(promptReadyTextColor_));
            if (version >= 0) archive(CEREAL_NVP(countTextColor_));
            if (version >= 0) archive(CEREAL_NVP(bombDimRate_));
            if (version >= 0) archive(CEREAL_NVP(readyFlashDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(readyPunchAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(readyPunchFrequency_));
            if (version >= 0) archive(CEREAL_NVP(readyPunchDamping_));
            if (version >= 0) archive(CEREAL_NVP(shockwaveDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(shockwaveStartRadius_));
            if (version >= 0) archive(CEREAL_NVP(shockwaveEndRadius_));
            if (version >= 0) archive(CEREAL_NVP(emberCount_));
            if (version >= 0) archive(CEREAL_NVP(emberDistance_));
            if (version >= 0) archive(CEREAL_NVP(sparkPopDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(wobbleAngle_deg_));
            if (version >= 0) archive(CEREAL_NVP(wobbleFrequency_hz_));
            if (version >= 0) archive(CEREAL_NVP(haloPulseFrequency_hz_));
            if (version >= 0) archive(CEREAL_NVP(drainDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(launchDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(launchOffset_));
            if (version >= 0) archive(CEREAL_NVP(recoilAmplitude_));
            if (version >= 1) archive(CEREAL_NVP(uiSounds_));
            if (version >= 2) archive(CEREAL_NVP(bombBrighten_secs_));
            if (version >= 2) archive(CEREAL_NVP(wobbleSettle_secs_));
            if (version >= 2) archive(CEREAL_NVP(countFadeOut_secs_));
            if (version >= 2) archive(CEREAL_NVP(countPopIn_secs_));
            if (version >= 2) archive(CEREAL_NVP(bombFadeIn_secs_));
            if (version >= 2) archive(CEREAL_NVP(launchShrinkRate_));
            if (version >= 2) archive(CEREAL_NVP(recoilFrequency_));
            if (version >= 2) archive(CEREAL_NVP(recoilDamping_));
            if (version >= 2) archive(CEREAL_NVP(emberStartAngle_deg_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::CannonCooldownGauge, 2);

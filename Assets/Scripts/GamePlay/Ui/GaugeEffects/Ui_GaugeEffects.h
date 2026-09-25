#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

namespace GamePlay::Ui
{
    // 同じ GameObject の Slider に HP ゲージの演出（トレイル・回復の光・満タンの光・ゲージ画像のクロスフェード・パルス・目盛り・先端の光）を重ねて描く。
    // Slider より上に描くので renderOrder_ は Slider より大きく、上に来る枠などより小さくする
    class GaugeEffects final : public Component::ComponentBase,
                               public LifeCycleCallback::IUserInterfaceRenderable,
                               public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }
        // トレイルを残さず、今の値にそろえる
        void SnapTrail();
        // ゲージ画像を gaugeFadeDuration_secs_ かけて切り替える（isStretchToDrawSize_ の Slider のみクロスフェード）
        void ChangeGaugeSprite(const std::shared_ptr<Asset::SpriteFile>& sprite);
        void SetPulse(bool isPulsing);

    private:
        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        void OnDrawGui() override;

        [[nodiscard]] std::shared_ptr<NanamiUi::Slider> FindSlider();
        void UpdateTrail(float value, float deltaTime);
        void StartHealTrail(float fromValue);
        void StartFullEffect();
        void DrawHealTrail(const NanamiUi::Slider& slider, float value) const;
        void DrawFullEffect(const NanamiUi::Slider& slider, float value) const;
        void DrawBand(const NanamiUi::Slider& slider, float alongMin, float alongMax, float acrossMin, float acrossMax, const Color32& color) const;
        void DrawTicks(const NanamiUi::Slider& slider) const;
        void DrawTip(const NanamiUi::Slider& slider, float value) const;

        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] FIELD(Asset::SpriteFile) trailSprite_;
        [[serialize(0)]] float trailDelay_secs_ = 0.5f;
        [[serialize(0)]] float trailDuration_secs_ = 0.8f;
        [[serialize(0)]] FIELD(Asset::SpriteFile) tipSprite_;
        [[serialize(0)]] float tipWidth_ = 18.0f;
        [[serialize(0)]] int tickCount_ = 0;
        // 目盛りの線は帯（bandInsetY_）からさらにこの分だけ内側に描く
        [[serialize(0)]] float tickInsetY_ = 1.5f;
        [[serialize(0)]] int tickShadowAlpha_ = 97;
        [[serialize(0)]] int tickHighlightAlpha_ = 26;
        // 目盛りと先端の光を描く帯の、伸びる方向と直交する両側の余白
        [[serialize(0)]] float bandInsetY_ = 0.0f;
        [[serialize(0)]] float gaugeFadeDuration_secs_ = 0.3f;
        [[serialize(0)]] float pulseFrequency_hz_ = 1.4f;
        [[serialize(0)]] int pulseMaxAlpha_ = 56;
        // 回復した範囲をゲージ画像の加算で光らせ、下端を今の値へ寄せながら消す
        [[serialize(1)]] Color32 healTrailColor_ = Color32(120, 255, 190);
        [[serialize(1)]] int healTrailAlpha_ = 200;
        [[serialize(1)]] float healTrailHold_secs_ = 0.25f;
        [[serialize(1)]] float healTrailDuration_secs_ = 0.6f;
        [[serialize(1)]] float healEdgeWidth_ = 3.0f;
        // 満タンになった瞬間にゲージ全体を光らせ、光の筋を始端から終端へ流す
        [[serialize(1)]] Color32 fullFlashColor_ = Color32(255, 250, 225);
        [[serialize(1)]] int fullFlashAlpha_ = 150;
        [[serialize(1)]] float fullFlashDuration_secs_ = 0.5f;
        [[serialize(1)]] float fullShineWidth_ = 48.0f;
        [[serialize(1)]] int fullShineAlpha_ = 220;
        [[serialize(1)]] float fullShineDuration_secs_ = 0.55f;

        std::weak_ptr<NanamiUi::Slider> slider_;
        bool hasObservedValue_ = false;
        float lastValue_ = 1.0f;
        float trailValue_ = 1.0f;
        float trailFrom_ = 1.0f;
        // trailFrom_ から Slider の値への進み具合(0..1)。値は回復で動くので値そのものではなく割合を補間する
        LibCore::Tween::TweenPlayer<float> trailTween_;
        float healFrom_ = 1.0f;
        // 0..1。回復の光の下端が healFrom_ から今の値へ寄った割合
        LibCore::Tween::TweenPlayer<float> healTween_;
        // 0..1。満タン演出の経過割合
        LibCore::Tween::TweenPlayer<float> fullTween_;
        std::shared_ptr<Asset::SpriteFile> fadingOutGaugeSprite_;
        LibCore::Tween::TweenPlayer<float> gaugeFadeTween_;
        bool isPulsing_ = false;
        float pulseTime_secs_ = 0.0f;

#pragma region Serialization
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(trailSprite_));
            archive(CEREAL_NVP(trailDelay_secs_));
            archive(CEREAL_NVP(trailDuration_secs_));
            archive(CEREAL_NVP(tipSprite_));
            archive(CEREAL_NVP(tipWidth_));
            archive(CEREAL_NVP(tickCount_));
            archive(CEREAL_NVP(tickInsetY_));
            archive(CEREAL_NVP(tickShadowAlpha_));
            archive(CEREAL_NVP(tickHighlightAlpha_));
            archive(CEREAL_NVP(bandInsetY_));
            archive(CEREAL_NVP(gaugeFadeDuration_secs_));
            archive(CEREAL_NVP(pulseFrequency_hz_));
            archive(CEREAL_NVP(pulseMaxAlpha_));
            archive(CEREAL_NVP(healTrailColor_));
            archive(CEREAL_NVP(healTrailAlpha_));
            archive(CEREAL_NVP(healTrailHold_secs_));
            archive(CEREAL_NVP(healTrailDuration_secs_));
            archive(CEREAL_NVP(healEdgeWidth_));
            archive(CEREAL_NVP(fullFlashColor_));
            archive(CEREAL_NVP(fullFlashAlpha_));
            archive(CEREAL_NVP(fullFlashDuration_secs_));
            archive(CEREAL_NVP(fullShineWidth_));
            archive(CEREAL_NVP(fullShineAlpha_));
            archive(CEREAL_NVP(fullShineDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(trailSprite_));
            if (version >= 0) archive(CEREAL_NVP(trailDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(trailDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(tipSprite_));
            if (version >= 0) archive(CEREAL_NVP(tipWidth_));
            if (version >= 0) archive(CEREAL_NVP(tickCount_));
            if (version >= 0) archive(CEREAL_NVP(tickInsetY_));
            if (version >= 0) archive(CEREAL_NVP(tickShadowAlpha_));
            if (version >= 0) archive(CEREAL_NVP(tickHighlightAlpha_));
            if (version >= 0) archive(CEREAL_NVP(bandInsetY_));
            if (version >= 0) archive(CEREAL_NVP(gaugeFadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(pulseFrequency_hz_));
            if (version >= 0) archive(CEREAL_NVP(pulseMaxAlpha_));
            if (version >= 1) archive(CEREAL_NVP(healTrailColor_));
            if (version >= 1) archive(CEREAL_NVP(healTrailAlpha_));
            if (version >= 1) archive(CEREAL_NVP(healTrailHold_secs_));
            if (version >= 1) archive(CEREAL_NVP(healTrailDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(healEdgeWidth_));
            if (version >= 1) archive(CEREAL_NVP(fullFlashColor_));
            if (version >= 1) archive(CEREAL_NVP(fullFlashAlpha_));
            if (version >= 1) archive(CEREAL_NVP(fullFlashDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(fullShineWidth_));
            if (version >= 1) archive(CEREAL_NVP(fullShineAlpha_));
            if (version >= 1) archive(CEREAL_NVP(fullShineDuration_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::GaugeEffects, 1);

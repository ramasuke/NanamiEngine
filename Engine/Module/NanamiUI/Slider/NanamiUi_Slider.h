#pragma once
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../../Component/ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../LifeCycleCallback/Update/IUpdatable.h"

namespace NanamiEngine::Module::NanamiUi
{
    class Slider final : public Component::ComponentBase,
                         public LifeCycleCallback::IUserInterfaceRenderable,
                         public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] float GetValue() const { return value_; }
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }
        void SetValue(float value);
        // ゲージ画像を gaugeFadeDuration_secs_ かけて切り替える（isStretchToDrawSize_ 時のみクロスフェード）
        void ChangeGaugeSprite(const std::shared_ptr<Asset::SpriteFile>& sprite);
        void SetPulse(bool isPulsing);

    private:
        struct DrawRect
        {
            int x;
            int y;
            int w;
            int h;
        };

        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        void OnDrawGui() override;

        [[nodiscard]] DrawRect CalcDrawRect() const;
        void DrawLayer(const DrawRect& rect, int graphHandle, float fillRate) const;
        void DrawMaskedGauge(const DrawRect& rect) const;
        void DrawTicks(const DrawRect& rect) const;
        void DrawTip(const DrawRect& rect) const;

        [[serialize(0)]] FIELD(Asset::SpriteFile) gaugeSprite_;
        [[serialize(0)]] glm::vec2 drawPosition_  = glm::vec2(0.0f, 0.0f);
        [[serialize(0)]] glm::vec2 drawSize_ = glm::vec2(100.0f, 20.0f);
        [[serialize(0)]] float value_ = 1.0f;
        [[serialize(0)]] int renderOrder_ = 0;

        // true: 画像を drawSize_ いっぱいに伸縮して描く / false: 従来どおり drawPosition_ 中心・等倍率で切り抜く
        [[serialize(1)]] bool isStretchToDrawSize_ = false;
        [[serialize(1)]] FIELD(Asset::SpriteFile) backgroundSprite_;
        [[serialize(1)]] FIELD(Asset::SpriteFile) trailSprite_;
        [[serialize(1)]] float trailDelay_secs_ = 0.5f;
        [[serialize(2)]] float trailDuration_secs_ = 0.8f;
        [[serialize(1)]] FIELD(Asset::SpriteFile) tipSprite_;
        [[serialize(1)]] float tipWidth_ = 18.0f;
        [[serialize(1)]] int tickCount_ = 0;
        // 目盛りの線は帯（bandInsetY_）からさらにこの分だけ内側に描く
        [[serialize(1)]] float tickInsetY_ = 1.5f;
        [[serialize(1)]] int tickShadowAlpha_ = 97;
        [[serialize(1)]] int tickHighlightAlpha_ = 26;
        // 目盛りと先端の光を描く帯の上下の余白（drawSize_ の上端・下端から）
        [[serialize(1)]] float bandInsetY_ = 0.0f;
        [[serialize(1)]] float gaugeFadeDuration_secs_ = 0.3f;
        [[serialize(1)]] float pulseFrequency_hz_ = 1.4f;
        [[serialize(1)]] int pulseMaxAlpha_ = 56;

        float trailValue_ = 1.0f;
        float trailFrom_ = 1.0f;
        float trailWaitTimer_secs_ = 0.0f;
        float trailElapsed_secs_ = 0.0f;
        std::shared_ptr<Asset::SpriteFile> fadingOutGaugeSprite_;
        float gaugeFadeTimer_secs_ = 0.0f;
        bool isPulsing_ = false;
        float pulseTime_secs_ = 0.0f;

#pragma region Serialization
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(gaugeSprite_));
            archive(CEREAL_NVP(drawPosition_));
            archive(CEREAL_NVP(drawSize_));
            archive(CEREAL_NVP(value_));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(isStretchToDrawSize_));
            archive(CEREAL_NVP(backgroundSprite_));
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
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(gaugeSprite_));
            if (version >= 0) archive(CEREAL_NVP(drawPosition_));
            if (version >= 0) archive(CEREAL_NVP(drawSize_));
            if (version >= 0) archive(CEREAL_NVP(value_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 1) archive(CEREAL_NVP(isStretchToDrawSize_));
            if (version >= 1) archive(CEREAL_NVP(backgroundSprite_));
            if (version >= 1) archive(CEREAL_NVP(trailSprite_));
            if (version >= 1) archive(CEREAL_NVP(trailDelay_secs_));
            if (version >= 2) archive(CEREAL_NVP(trailDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(tipSprite_));
            if (version >= 1) archive(CEREAL_NVP(tipWidth_));
            if (version >= 1) archive(CEREAL_NVP(tickCount_));
            if (version >= 1) archive(CEREAL_NVP(tickInsetY_));
            if (version >= 1) archive(CEREAL_NVP(tickShadowAlpha_));
            if (version >= 1) archive(CEREAL_NVP(tickHighlightAlpha_));
            if (version >= 1) archive(CEREAL_NVP(bandInsetY_));
            if (version >= 1) archive(CEREAL_NVP(gaugeFadeDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(pulseFrequency_hz_));
            if (version >= 1) archive(CEREAL_NVP(pulseMaxAlpha_));
            trailValue_ = value_;
            trailFrom_  = value_;
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiUi::Slider, 2)
#pragma once
#include <array>
#include <string_view>
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../../Component/ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../LifeCycleCallback/Update/IUpdatable.h"

namespace NanamiEngine::Module::NanamiUi
{
    enum class SliderFillDirection : int
    {
        LeftToRight = 0,
        RightToLeft = 1,
        BottomToTop = 2,
        TopToBottom = 3,
    };

    constexpr std::array SLIDER_FILL_DIRECTIONS
    {
        SliderFillDirection::LeftToRight,
        SliderFillDirection::RightToLeft,
        SliderFillDirection::BottomToTop,
        SliderFillDirection::TopToBottom,
    };

    constexpr std::string_view ToString(const SliderFillDirection direction)
    {
        switch (direction)
        {
        case SliderFillDirection::LeftToRight: return "LeftToRight";
        case SliderFillDirection::RightToLeft: return "RightToLeft";
        case SliderFillDirection::BottomToTop: return "BottomToTop";
        case SliderFillDirection::TopToBottom: return "TopToBottom";
        }
        return "Unknown";
    }

    // isStretchToDrawSize_ 時は Transform の位置を drawSize_ の左上とし、Transform の Z 回転でそこを中心に回す。
    // fillDirection_ / fillStartInset_ / fillEndInset_ と回転が効くのはこのモードだけ
    class Slider final : public Component::ComponentBase,
                         public LifeCycleCallback::IUserInterfaceRenderable,
                         public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] float GetValue() const { return value_; }
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }
        void SetValue(float value);
        // トレイルを残さずに値を切り替える
        void SetValueImmediate(float value);
        // ゲージ画像を gaugeFadeDuration_secs_ かけて切り替える（isStretchToDrawSize_ 時のみクロスフェード）
        void ChangeGaugeSprite(const std::shared_ptr<Asset::SpriteFile>& sprite);
        void SetPulse(bool isPulsing);

    private:
        struct DrawFrame
        {
            glm::vec2 origin;
            glm::vec2 axisX;
            glm::vec2 axisY;
            glm::vec2 size;
            bool isRotated;

            [[nodiscard]] glm::vec2 ToScreen(const glm::vec2& local) const { return origin + axisX * local.x + axisY * local.y; }
        };

        // drawSize_ 内の座標（左上原点）の矩形
        struct LocalRect
        {
            glm::vec2 min;
            glm::vec2 max;
        };

        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        void OnDrawGui() override;

        [[nodiscard]] DrawFrame CalcDrawFrame() const;
        [[nodiscard]] bool IsVerticalFill() const;
        [[nodiscard]] float AlongLength() const;
        [[nodiscard]] float AcrossLength() const;
        [[nodiscard]] float CalcFillLength(float fillRate) const;
        // along: 伸びる方向に始端からの距離 / across: それと直交する方向の距離
        [[nodiscard]] glm::vec2 FillToLocal(float along, float across) const;
        [[nodiscard]] LocalRect FillToLocalRect(float alongMin, float alongMax, float acrossMin, float acrossMax) const;
        // 回転していないときだけ drawSize_ の範囲でクリップする（回転時の各層はクリップを使わない）
        void ClipToDrawSize(const DrawFrame& frame) const;
        void DrawLayer(const DrawFrame& frame, int graphHandle, float fillRate) const;
        void DrawStretchedLayer(const DrawFrame& frame, int graphHandle, float fillRate) const;
        void DrawUnstretchedLayer(const DrawFrame& frame, int graphHandle, float fillRate) const;
        void DrawMaskedGauge(const DrawFrame& frame) const;
        void DrawTicks(const DrawFrame& frame) const;
        void DrawTip(const DrawFrame& frame) const;
        void FillLocalRect(const DrawFrame& frame, const LocalRect& rect, unsigned int color) const;

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
        // 目盛りと先端の光を描く帯の、伸びる方向と直交する両側の余白
        [[serialize(1)]] float bandInsetY_ = 0.0f;
        [[serialize(1)]] float gaugeFadeDuration_secs_ = 0.3f;
        [[serialize(1)]] float pulseFrequency_hz_ = 1.4f;
        [[serialize(1)]] int pulseMaxAlpha_ = 56;
        [[serialize(3)]] SliderFillDirection fillDirection_ = SliderFillDirection::LeftToRight;
        // value_ 0〜1 を伸びる方向の [fillStartInset_, 長さ - fillEndInset_] に対応させる（画像の透明な余白用）。1 のときは全体を描く
        [[serialize(3)]] float fillStartInset_ = 0.0f;
        [[serialize(3)]] float fillEndInset_ = 0.0f;

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
            archive(CEREAL_NVP(fillDirection_));
            archive(CEREAL_NVP(fillStartInset_));
            archive(CEREAL_NVP(fillEndInset_));
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
            if (version >= 3) archive(CEREAL_NVP(fillDirection_));
            if (version >= 3) archive(CEREAL_NVP(fillStartInset_));
            if (version >= 3) archive(CEREAL_NVP(fillEndInset_));
            trailValue_ = value_;
            trailFrom_  = value_;
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiUi::Slider, 3)
#pragma once
#include <array>
#include <string_view>
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../../Component/ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"

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

    // Unity の Slider 相当
    class Slider final : public Component::ComponentBase,
                         public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        [[nodiscard]] float GetValue() const { return value_; }
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }
        void SetValue(float value);
        [[nodiscard]] std::shared_ptr<Asset::SpriteFile> GetGaugeSprite() const { return gaugeSprite_.get(); }
        void SetGaugeSprite(const std::shared_ptr<Asset::SpriteFile>& sprite) { gaugeSprite_ = sprite; }

        // 与えた画像を、塗りと同じ伸縮・クリップで fromRate〜toRate の区間だけ描く（ブレンドモードは呼び出し側の設定を使う）
        void DrawFillRange(int graphHandle, float fromRate, float toRate) const;
        // along: 伸びる方向に始端からの距離 / across: それと直交する方向の距離
        [[nodiscard]] glm::vec2 FillToScreen(float along, float across) const;
        [[nodiscard]] float CalcFillLength(float fillRate) const;
        [[nodiscard]] float AlongLength() const;
        [[nodiscard]] float AcrossLength() const;
        [[nodiscard]] float GetFillStartInset() const { return fillStartInset_; }
        [[nodiscard]] float GetFillEndInset() const { return fillEndInset_; }
        [[nodiscard]] bool IsRotated() const { return CalcDrawFrame().isRotated; }
        [[nodiscard]] bool IsStretchToDrawSize() const { return isStretchToDrawSize_; }
        // 回転していないときだけ drawSize_ の範囲でクリップする（画面全体に戻すのは呼び出し側）
        void ClipToDrawSize() const;

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

        void OnUserInterfaceRender() override;
        void OnDrawGui() override;

        [[nodiscard]] DrawFrame CalcDrawFrame() const;
        [[nodiscard]] bool IsVerticalFill() const;
        [[nodiscard]] glm::vec2 FillToLocal(float along, float across) const;
        [[nodiscard]] LocalRect FillToLocalRect(float alongMin, float alongMax, float acrossMin, float acrossMax) const;
        void DrawLayer(const DrawFrame& frame, int graphHandle, float fromRate, float toRate) const;
        void DrawStretchedLayer(const DrawFrame& frame, int graphHandle, float fromRate, float toRate) const;
        void DrawUnstretchedLayer(const DrawFrame& frame, int graphHandle, float fromRate, float toRate) const;
        void DrawMaskedGauge(const DrawFrame& frame) const;

        [[serialize(0)]] FIELD(Asset::SpriteFile) gaugeSprite_;
        [[serialize(0)]] glm::vec2 drawPosition_  = glm::vec2(0.0f, 0.0f);
        [[serialize(0)]] glm::vec2 drawSize_ = glm::vec2(100.0f, 20.0f);
        [[serialize(0)]] float value_ = 1.0f;
        [[serialize(0)]] int renderOrder_ = 0;

        // true: 画像を drawSize_ いっぱいに伸縮して描く / false: 従来どおり drawPosition_ 中心・等倍率で切り抜く
        [[serialize(1)]] bool isStretchToDrawSize_ = false;
        [[serialize(1)]] FIELD(Asset::SpriteFile) backgroundSprite_;
        [[serialize(3)]] SliderFillDirection fillDirection_ = SliderFillDirection::LeftToRight;
        [[serialize(3)]] float fillStartInset_ = 0.0f;
        [[serialize(3)]] float fillEndInset_ = 0.0f;

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
            // v4 でトレイル・先端・目盛り・パルス・クロスフェードを GaugeEffects（ゲーム側）へ移したので読み捨てる
            if (version >= 1 && version <= 3)
            {
                FIELD(Asset::SpriteFile) legacyTrailSprite;
                float legacyTrailDelay_secs = 0.0f;
                float legacyTrailDuration_secs = 0.0f;
                FIELD(Asset::SpriteFile) legacyTipSprite;
                float legacyTipWidth = 0.0f;
                int legacyTickCount = 0;
                float legacyTickInsetY = 0.0f;
                int legacyTickShadowAlpha = 0;
                int legacyTickHighlightAlpha = 0;
                float legacyBandInsetY = 0.0f;
                float legacyGaugeFadeDuration_secs = 0.0f;
                float legacyPulseFrequency_hz = 0.0f;
                int legacyPulseMaxAlpha = 0;
                archive(cereal::make_nvp("trailSprite_", legacyTrailSprite));
                archive(cereal::make_nvp("trailDelay_secs_", legacyTrailDelay_secs));
                if (version >= 2) archive(cereal::make_nvp("trailDuration_secs_", legacyTrailDuration_secs));
                archive(cereal::make_nvp("tipSprite_", legacyTipSprite));
                archive(cereal::make_nvp("tipWidth_", legacyTipWidth));
                archive(cereal::make_nvp("tickCount_", legacyTickCount));
                archive(cereal::make_nvp("tickInsetY_", legacyTickInsetY));
                archive(cereal::make_nvp("tickShadowAlpha_", legacyTickShadowAlpha));
                archive(cereal::make_nvp("tickHighlightAlpha_", legacyTickHighlightAlpha));
                archive(cereal::make_nvp("bandInsetY_", legacyBandInsetY));
                archive(cereal::make_nvp("gaugeFadeDuration_secs_", legacyGaugeFadeDuration_secs));
                archive(cereal::make_nvp("pulseFrequency_hz_", legacyPulseFrequency_hz));
                archive(cereal::make_nvp("pulseMaxAlpha_", legacyPulseMaxAlpha));
            }
            if (version >= 3) archive(CEREAL_NVP(fillDirection_));
            if (version >= 3) archive(CEREAL_NVP(fillStartInset_));
            if (version >= 3) archive(CEREAL_NVP(fillEndInset_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiUi::Slider, 4);

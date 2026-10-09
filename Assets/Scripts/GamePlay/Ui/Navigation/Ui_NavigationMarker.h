#pragma once
#include <string>

#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Font/Ttf/TtfFontFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/LifeCycleCallback/UserInterfaceRenderable/IUserInterfaceRenderable.h"

namespace GamePlay::Ui
{
    // NOTE: 目的地の目印。画面内なら光の玉、画面外なら画面端に蛍の列を出す
    class NavigationMarker final : public Component::ComponentBase,
                                   public LifeCycleCallback::IUpdatable,
                                   public LifeCycleCallback::IUserInterfaceRenderable
    {
    private:
        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        void DrawOrb(const glm::vec2& position, float scale, float alpha) const;
        void DrawEdge(const glm::vec2& position, const glm::vec2& direction, float alpha) const;
        void DrawLabel(const glm::vec2& position, const std::string& name, const std::string& distance, bool alignRight, float alpha) const;
        [[nodiscard]] std::string DistanceText(float distance) const;

        [[serialize(0)]] int                        renderOrder_ = 0;
        [[serialize(0)]] FIELD(Asset::SpriteFile)   orbSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile)   fireflySprite_;
        [[serialize(0)]] FIELD(Asset::TtfFontFile)  font_;
        // NOTE: 距離の表示に使う。人の背丈がおよそ 19 ユニット
        [[serialize(0)]] float                      metersPerUnit_ = 0.1f;
        // NOTE: これより近い(ユニット)と目印を出さない。NPC なら頭上の驚きアイコンで足りる
        [[serialize(0)]] float                      hideDistance_ = 45.0f;
        [[serialize(0)]] float                      fadeDistance_ = 25.0f;
        [[serialize(0)]] float                      orbReferenceDistance_ = 120.0f;
        [[serialize(0)]] float                      orbMinScale_ = 0.35f;
        [[serialize(0)]] float                      orbMaxScale_ = 0.8f;
        [[serialize(0)]] float                      orbPulsePeriod_secs_ = 1.6f;
        [[serialize(0)]] float                      orbPulseRate_ = 0.12f;
        [[serialize(0)]] glm::vec2                  labelOffset_px_ = glm::vec2(18.0f, -14.0f);
        [[serialize(0)]] float                      nameScale_ = 0.33f;
        [[serialize(0)]] float                      distanceScale_ = 0.27f;
        [[serialize(0)]] float                      lineGap_px_ = 22.0f;
        [[serialize(0)]] Color32                    nameColor_ = Color32(255, 255, 247);
        [[serialize(0)]] Color32                    distanceColor_ = Color32(210, 228, 223);
        // NOTE: 画面端の蛍を置く、画面の縁からの距離
        [[serialize(0)]] float                      edgeMargin_px_ = 56.0f;
        [[serialize(0)]] int                        edgeFireflyCount_ = 3;
        [[serialize(0)]] float                      edgeFireflySpacing_px_ = 20.0f;
        [[serialize(0)]] float                      edgeFireflyScale_ = 0.7f;
        [[serialize(0)]] float                      edgeFlowPeriod_secs_ = 1.2f;
        [[serialize(0)]] float                      fade_secs_ = 0.3f;

        float time_secs_ = 0.0f;
        // NOTE: 出ている度合い 0..1。目的が無い・静かにする間は 0 へ
        float visibility_ = 0.0f;
        // NOTE: プレイヤーから目的地の足元まで(ユニット)
        float distance_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(orbSprite_));
            archive(CEREAL_NVP(fireflySprite_));
            archive(CEREAL_NVP(font_));
            archive(CEREAL_NVP(metersPerUnit_));
            archive(CEREAL_NVP(hideDistance_));
            archive(CEREAL_NVP(fadeDistance_));
            archive(CEREAL_NVP(orbReferenceDistance_));
            archive(CEREAL_NVP(orbMinScale_));
            archive(CEREAL_NVP(orbMaxScale_));
            archive(CEREAL_NVP(orbPulsePeriod_secs_));
            archive(CEREAL_NVP(orbPulseRate_));
            archive(CEREAL_NVP(labelOffset_px_));
            archive(CEREAL_NVP(nameScale_));
            archive(CEREAL_NVP(distanceScale_));
            archive(CEREAL_NVP(lineGap_px_));
            archive(CEREAL_NVP(nameColor_));
            archive(CEREAL_NVP(distanceColor_));
            archive(CEREAL_NVP(edgeMargin_px_));
            archive(CEREAL_NVP(edgeFireflyCount_));
            archive(CEREAL_NVP(edgeFireflySpacing_px_));
            archive(CEREAL_NVP(edgeFireflyScale_));
            archive(CEREAL_NVP(edgeFlowPeriod_secs_));
            archive(CEREAL_NVP(fade_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(orbSprite_));
            if (version >= 0) archive(CEREAL_NVP(fireflySprite_));
            if (version >= 0) archive(CEREAL_NVP(font_));
            if (version >= 0) archive(CEREAL_NVP(metersPerUnit_));
            if (version >= 0) archive(CEREAL_NVP(hideDistance_));
            if (version >= 0) archive(CEREAL_NVP(fadeDistance_));
            if (version >= 0) archive(CEREAL_NVP(orbReferenceDistance_));
            if (version >= 0) archive(CEREAL_NVP(orbMinScale_));
            if (version >= 0) archive(CEREAL_NVP(orbMaxScale_));
            if (version >= 0) archive(CEREAL_NVP(orbPulsePeriod_secs_));
            if (version >= 0) archive(CEREAL_NVP(orbPulseRate_));
            if (version >= 0) archive(CEREAL_NVP(labelOffset_px_));
            if (version >= 0) archive(CEREAL_NVP(nameScale_));
            if (version >= 0) archive(CEREAL_NVP(distanceScale_));
            if (version >= 0) archive(CEREAL_NVP(lineGap_px_));
            if (version >= 0) archive(CEREAL_NVP(nameColor_));
            if (version >= 0) archive(CEREAL_NVP(distanceColor_));
            if (version >= 0) archive(CEREAL_NVP(edgeMargin_px_));
            if (version >= 0) archive(CEREAL_NVP(edgeFireflyCount_));
            if (version >= 0) archive(CEREAL_NVP(edgeFireflySpacing_px_));
            if (version >= 0) archive(CEREAL_NVP(edgeFireflyScale_));
            if (version >= 0) archive(CEREAL_NVP(edgeFlowPeriod_secs_));
            if (version >= 0) archive(CEREAL_NVP(fade_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::NavigationMarker, 0);

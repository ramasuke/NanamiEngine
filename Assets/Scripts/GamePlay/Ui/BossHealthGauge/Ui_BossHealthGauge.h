#pragma once
#include "../../../../../Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    // 全ボス共通のHP表示。紋章の上に扇状に並べた結晶1本がHPの 1/shardCount_ にあたり、右端の結晶から先端側へ欠けていく。
    // 画像は tools/art/boss_health_gauge.py で生成し、配置の数値（shardBaseDistance_ など）もそのスクリプトの出力に合わせる
    class BossHealthGauge final : public Component::ComponentBase,
                                  public LifeCycleCallback::IUserInterfaceRenderable,
                                  public LifeCycleCallback::IUpdatable
    {
    public:
        void Show(const std::string& bossName);
        void SetHealthRate(float healthRate);

    private:
        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        void ApplyValue(float healthRate);
        [[nodiscard]] bool IsIntroPlaying() const;
        [[nodiscard]] bool IsDanger() const;
        void DrawShard(int graphHandle, float fillRate, const glm::vec2& pivot, float angle) const;

        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] std::string bossNameTextName_ = "BossName";
        [[serialize(0)]] FIELD(Asset::SpriteFile) crestSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) crestGlowSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) shardEmptySprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) shardFillSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) shardFillDangerSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) shardTrailSprite_;
        [[serialize(0)]] int shardCount_ = 8;
        [[serialize(0)]] float arcAngle_deg_ = 140.0f;
        // 回転中心から結晶の根元までの距離と、結晶画像の上下の余白
        [[serialize(0)]] float shardBaseDistance_ = 94.4f;
        [[serialize(0)]] float shardPadding_px_ = 10.4f;
        [[serialize(0)]] glm::vec2 pivotOffset_ = glm::vec2(0.0f, 84.8f);
        [[serialize(0)]] glm::vec2 crestOffset_ = glm::vec2(0.0f, -35.2f);
        [[serialize(0)]] float dangerHealthRate_ = 0.3f;
        [[serialize(0)]] float trailDelay_secs_ = 0.5f;
        [[serialize(0)]] float trailSpeed_perSec_ = 0.35f;
        [[serialize(0)]] float pulseFrequency_hz_ = 1.4f;
        [[serialize(0)]] int pulseMaxAlpha_ = 90;
        [[serialize(0)]] float introFillDuration_secs_ = 1.2f;

        FIELD(NanamiUi::TextRenderer) bossNameText_;
        float targetRate_ = 1.0f;
        float value_ = 0.0f;
        float trailValue_ = 0.0f;
        float trailWaitTimer_secs_ = 0.0f;
        float introElapsed_secs_ = 0.0f;
        float pulseTime_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(bossNameTextName_));
            archive(CEREAL_NVP(crestSprite_));
            archive(CEREAL_NVP(crestGlowSprite_));
            archive(CEREAL_NVP(shardEmptySprite_));
            archive(CEREAL_NVP(shardFillSprite_));
            archive(CEREAL_NVP(shardFillDangerSprite_));
            archive(CEREAL_NVP(shardTrailSprite_));
            archive(CEREAL_NVP(shardCount_));
            archive(CEREAL_NVP(arcAngle_deg_));
            archive(CEREAL_NVP(shardBaseDistance_));
            archive(CEREAL_NVP(shardPadding_px_));
            archive(CEREAL_NVP(pivotOffset_));
            archive(CEREAL_NVP(crestOffset_));
            archive(CEREAL_NVP(dangerHealthRate_));
            archive(CEREAL_NVP(trailDelay_secs_));
            archive(CEREAL_NVP(trailSpeed_perSec_));
            archive(CEREAL_NVP(pulseFrequency_hz_));
            archive(CEREAL_NVP(pulseMaxAlpha_));
            archive(CEREAL_NVP(introFillDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(bossNameTextName_));
            if (version >= 0) archive(CEREAL_NVP(crestSprite_));
            if (version >= 0) archive(CEREAL_NVP(crestGlowSprite_));
            if (version >= 0) archive(CEREAL_NVP(shardEmptySprite_));
            if (version >= 0) archive(CEREAL_NVP(shardFillSprite_));
            if (version >= 0) archive(CEREAL_NVP(shardFillDangerSprite_));
            if (version >= 0) archive(CEREAL_NVP(shardTrailSprite_));
            if (version >= 0) archive(CEREAL_NVP(shardCount_));
            if (version >= 0) archive(CEREAL_NVP(arcAngle_deg_));
            if (version >= 0) archive(CEREAL_NVP(shardBaseDistance_));
            if (version >= 0) archive(CEREAL_NVP(shardPadding_px_));
            if (version >= 0) archive(CEREAL_NVP(pivotOffset_));
            if (version >= 0) archive(CEREAL_NVP(crestOffset_));
            if (version >= 0) archive(CEREAL_NVP(dangerHealthRate_));
            if (version >= 0) archive(CEREAL_NVP(trailDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(trailSpeed_perSec_));
            if (version >= 0) archive(CEREAL_NVP(pulseFrequency_hz_));
            if (version >= 0) archive(CEREAL_NVP(pulseMaxAlpha_));
            if (version >= 0) archive(CEREAL_NVP(introFillDuration_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BossHealthGauge, 0)

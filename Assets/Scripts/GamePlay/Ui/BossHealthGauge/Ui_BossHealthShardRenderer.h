#pragma once
#include "../../../../../Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Ui
{
    // ボスHPの結晶1本。Transform の位置を回転中心、Z回転を向きとして、根元側から fillRate 分だけ切り出して描く。
    // 余白などの数値は tools/art/boss_health_gauge.py の出力（GEOMETRY）に合わせる
    class BossHealthShardRenderer final : public Component::ComponentBase,
                                          public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        void SetFill(float fillRate, float trailFillRate);
        void SetDanger(bool isDanger);

    private:
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        void DrawLayer(int graphHandle, float fillRate, const glm::vec2& pivot, float angle) const;

        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] FIELD(Asset::SpriteFile) emptySprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) fillSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) fillDangerSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) trailSprite_;
        // 回転中心から結晶の根元までの距離と、結晶画像の上下の余白
        [[serialize(0)]] float baseDistance_ = 94.4f;
        [[serialize(0)]] float padding_px_ = 10.4f;

        float fillRate_ = 1.0f;
        float trailFillRate_ = 0.0f;
        bool isDanger_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(emptySprite_));
            archive(CEREAL_NVP(fillSprite_));
            archive(CEREAL_NVP(fillDangerSprite_));
            archive(CEREAL_NVP(trailSprite_));
            archive(CEREAL_NVP(baseDistance_));
            archive(CEREAL_NVP(padding_px_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(emptySprite_));
            if (version >= 0) archive(CEREAL_NVP(fillSprite_));
            if (version >= 0) archive(CEREAL_NVP(fillDangerSprite_));
            if (version >= 0) archive(CEREAL_NVP(trailSprite_));
            if (version >= 0) archive(CEREAL_NVP(baseDistance_));
            if (version >= 0) archive(CEREAL_NVP(padding_px_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BossHealthShardRenderer, 0)

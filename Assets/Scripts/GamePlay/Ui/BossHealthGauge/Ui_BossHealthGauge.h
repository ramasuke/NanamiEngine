#pragma once
#include <vector>
#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    // 全ボス共通のHP表示。shards_ の結晶1本（Slider）がHPの 1/結晶数 にあたり、右端（最後の要素）の結晶から欠けていく。
    // 描画は子オブジェクトの Slider / ImageRenderer / BlendImageRenderer / TextRenderer が行う
    class BossHealthGauge final : public Component::ComponentBase,
                                  public LifeCycleCallback::IUpdatable
    {
    public:
        void Show(const std::string& bossName);
        void SetHealthRate(float healthRate);

    private:
        void OnUpdate() override;

        void ApplyToRenderers();
        [[nodiscard]] bool IsIntroPlaying() const;
        [[nodiscard]] bool IsDanger() const;

        [[serialize(2)]] FIELD(NanamiUi::TextRenderer) bossNameText_;
        [[serialize(3)]] std::vector<FIELD(NanamiUi::Slider)> shards_;
        [[serialize(2)]] FIELD(NanamiUi::BlendImageRenderer) crestGlow_;
        [[serialize(3)]] FIELD(Asset::SpriteFile) fillSprite_;
        [[serialize(3)]] FIELD(Asset::SpriteFile) fillDangerSprite_;
        [[serialize(0)]] float dangerHealthRate_ = 0.3f;
        [[serialize(0)]] float pulseFrequency_hz_ = 1.4f;
        [[serialize(0)]] int pulseMaxAlpha_ = 90;
        [[serialize(0)]] float introFillDuration_secs_ = 1.2f;

        float targetRate_ = 1.0f;
        float value_ = 0.0f;
        float introElapsed_secs_ = 0.0f;
        float pulseTime_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(bossNameText_));
            archive(CEREAL_NVP(shards_));
            archive(CEREAL_NVP(crestGlow_));
            archive(CEREAL_NVP(fillSprite_));
            archive(CEREAL_NVP(fillDangerSprite_));
            archive(CEREAL_NVP(dangerHealthRate_));
            archive(CEREAL_NVP(pulseFrequency_hz_));
            archive(CEREAL_NVP(pulseMaxAlpha_));
            archive(CEREAL_NVP(introFillDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            // v3 で結晶を Slider にし、トレイルを各 Slider に任せた
            FIELD(GameObject::IGameObject) shardsObject_;
            float trailDelay_secs_ = 0.0f;
            float trailSpeed_perSec_ = 0.0f;
            if (version >= 2) archive(CEREAL_NVP(bossNameText_));
            if (version == 2) archive(CEREAL_NVP(shardsObject_));
            if (version >= 3) archive(CEREAL_NVP(shards_));
            if (version >= 2) archive(CEREAL_NVP(crestGlow_));
            if (version >= 3) archive(CEREAL_NVP(fillSprite_));
            if (version >= 3) archive(CEREAL_NVP(fillDangerSprite_));
            if (version >= 0) archive(CEREAL_NVP(dangerHealthRate_));
            if (version < 3) archive(CEREAL_NVP(trailDelay_secs_));
            if (version < 3) archive(CEREAL_NVP(trailSpeed_perSec_));
            if (version >= 0) archive(CEREAL_NVP(pulseFrequency_hz_));
            if (version >= 0) archive(CEREAL_NVP(pulseMaxAlpha_));
            if (version >= 0) archive(CEREAL_NVP(introFillDuration_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BossHealthGauge, 3)

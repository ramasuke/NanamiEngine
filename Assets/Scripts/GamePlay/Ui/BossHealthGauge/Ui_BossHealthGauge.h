#pragma once
#include <vector>
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Ui_BossHealthShardRenderer.h"

namespace GamePlay::Ui
{
    // 全ボス共通のHP表示。shardsObject_ の子に並べた結晶1本がHPの 1/結晶数 にあたり、右端（最後の子）の結晶から欠けていく。
    // 描画は子オブジェクトの BossHealthShardRenderer / ImageRenderer / BlendImageRenderer / TextRenderer が行う
    class BossHealthGauge final : public Component::ComponentBase,
                                  public LifeCycleCallback::IUpdatable
    {
    public:
        void Show(const std::string& bossName);
        void SetHealthRate(float healthRate);

    private:
        void OnUpdate() override;

        void CatchParts();
        void ApplyValue(float healthRate);
        void ApplyToRenderers();
        [[nodiscard]] bool IsIntroPlaying() const;
        [[nodiscard]] bool IsDanger() const;

        [[serialize(2)]] FIELD(NanamiUi::TextRenderer) bossNameText_;
        [[serialize(2)]] FIELD(GameObject::IGameObject) shardsObject_;
        [[serialize(2)]] FIELD(NanamiUi::BlendImageRenderer) crestGlow_;
        [[serialize(0)]] float dangerHealthRate_ = 0.3f;
        [[serialize(0)]] float trailDelay_secs_ = 0.5f;
        [[serialize(0)]] float trailSpeed_perSec_ = 0.35f;
        [[serialize(0)]] float pulseFrequency_hz_ = 1.4f;
        [[serialize(0)]] int pulseMaxAlpha_ = 90;
        [[serialize(0)]] float introFillDuration_secs_ = 1.2f;

        std::vector<std::weak_ptr<BossHealthShardRenderer>> shards_;
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
            archive(CEREAL_NVP(bossNameText_));
            archive(CEREAL_NVP(shardsObject_));
            archive(CEREAL_NVP(crestGlow_));
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
            // v2 で子オブジェクトの名前検索を FIELD に置き換えた
            std::string bossNameTextName_, shardsName_, crestGlowName_;
            if (version < 2) archive(CEREAL_NVP(bossNameTextName_));
            if (version >= 2) archive(CEREAL_NVP(bossNameText_));
            if (version >= 1 && version < 2) archive(CEREAL_NVP(shardsName_));
            if (version >= 2) archive(CEREAL_NVP(shardsObject_));
            if (version >= 1 && version < 2) archive(CEREAL_NVP(crestGlowName_));
            if (version >= 2) archive(CEREAL_NVP(crestGlow_));
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

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BossHealthGauge, 2)

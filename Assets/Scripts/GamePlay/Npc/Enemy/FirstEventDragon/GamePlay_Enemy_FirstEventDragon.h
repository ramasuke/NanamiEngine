#pragma once
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../Core/Game/Npc/Enemy/Boss/BossEnemyBase.h"
#include "../../../Ui/BossHealthGauge/Ui_BossHealthGauge.h"

namespace GamePlay::Npc::Enemy
{
    class FirstEventDragon final : public GameCore::Npc::BossEnemyBase
    {
    private:
        void DoUpdate() override;
        // NOTE: EnemyFactory の NormalBoss はこの竜のプレハブ
        [[nodiscard]] std::optional<GameCore::Npc::Enemy::EnemyKind> RecordKind() const override { return GameCore::Npc::Enemy::EnemyKind::NormalBoss; }

        // NOTE: 島の外へ落ちたら戻す位置
        [[serialize(5)]] glm::vec3 respawnPosition_ = glm::vec3(0.0f, 300.0f, 0.0f);
        // NOTE: これより下へ落ちたら戻す
        [[serialize(5)]] float fallLimitY_ = -100.0f;

#pragma region Serialization Function
    public:
        void BasedOnDrawgui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<BossEnemyBase>(this));
            archive(CEREAL_NVP(respawnPosition_));
            archive(CEREAL_NVP(fallLimitY_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version <= 3)
            {
                archive(cereal::base_class<EnemyBase>(this));
                [[serialize(1)]] FIELD(NanamiUi::Slider) healthBar_;
                if (version == 1) archive(CEREAL_NVP(healthBar_));
                [[serialize(2)]] FIELD(Ui::BossHealthGauge) bossHealthGauge_;
                if (version == 2) archive(CEREAL_NVP(bossHealthGauge_));
                LoadLegacyBossFields(archive, version);
            }
            else archive(cereal::base_class<BossEnemyBase>(this));
            if (version >= 5) archive(CEREAL_NVP(respawnPosition_));
            if (version >= 5) archive(CEREAL_NVP(fallLimitY_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::FirstEventDragon, 5);
#pragma endregion

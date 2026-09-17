#pragma once
#include "../../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../Core/Game/Npc/Enemy/Boss/BossEnemyBase.h"
#include "../../../Ui/BossHealthGauge/Ui_BossHealthGauge.h"

namespace GamePlay::Npc::Enemy
{
    class FirstEventDragon final : public GameCore::Npc::BossEnemyBase
    {
    private:
        void DoUpdate() override;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<BossEnemyBase>(this));
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
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::FirstEventDragon, 4);
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::FirstEventDragon);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::BossEnemyBase, GamePlay::Npc::Enemy::FirstEventDragon);
#pragma endregion
#pragma once
#include "../../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../Core/Game/Npc/Enemy/EnemyBase.h"
#include "../../../Ui/BossHealthGauge/Ui_BossHealthGauge.h"

namespace GamePlay::Npc::Enemy
{
    class Tyrannosaurus final : public GameCore::Npc::EnemyBase
    {
    private:
        void DoAwake() override;
        void DoUpdate() override;

        [[serialize(2)]] FIELD(Ui::BossHealthGauge) bossHealthGauge_;
        [[serialize(2)]] std::string bossName_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<EnemyBase>(this));
            archive(CEREAL_NVP(bossHealthGauge_));
            archive(CEREAL_NVP(bossName_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<EnemyBase>(this));
            [[serialize(1)]] FIELD(NanamiUi::Slider) healthBar_;
            if (version == 1) archive(CEREAL_NVP(healthBar_));
            if (version >= 2) archive(CEREAL_NVP(bossHealthGauge_));
            if (version >= 2) archive(CEREAL_NVP(bossName_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::Tyrannosaurus, 2);
CEREAL_REGISTER_TYPE(GamePlay::Npc::Enemy::Tyrannosaurus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::EnemyBase, GamePlay::Npc::Enemy::Tyrannosaurus);
#pragma endregion

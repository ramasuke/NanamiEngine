#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../Core/Game/Npc/Enemy/Boss/BossEnemyBase.h"
#include "../../../Ui/BossHealthGauge/Ui_BossHealthGauge.h"

namespace GamePlay::Npc::Enemy
{
    class Tyrannosaurus final : public GameCore::Npc::BossEnemyBase
    {
    private:
        struct FootLatch
        {
            bool                 armed = false;
            std::optional<float> prevHeight;
        };

        [[nodiscard]] std::optional<GameCore::Npc::Enemy::EnemyKind> RecordKind() const override { return GameCore::Npc::Enemy::EnemyKind::Tyrannosaurus; }
        void DoUpdate() override;
        /** @brief 足が着地した瞬間にローカルプレイヤーとの距離で減衰させたカメラシェイクを掛ける */
        void TryEmitFootQuake();
        void EmitFootQuake(const glm::vec3& stepPos) const;

        [[serialize(5)]] std::vector<std::string> footBoneNames_ = { "jt_Foot_L", "jt_Foot_R" };
        /** 足元からこの高さより上がった足が降りてきたら着地とみなす */
        [[serialize(5)]] float footContactHeight_ = 75.0f;
        [[serialize(5)]] float footQuakeIntensity_ = 0.5f;
        [[serialize(5)]] float footQuakeDuration_secs_ = 0.25f;
        /** この距離までは最大強度、footQuakeOuterRadius_ で 0 */
        [[serialize(5)]] float footQuakeInnerRadius_ = 800.0f;
        [[serialize(5)]] float footQuakeOuterRadius_ = 3000.0f;
        [[serialize(5)]] FIELD(Asset::SoundFile) footstepSound_;
        std::vector<FootLatch> footLatches_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<BossEnemyBase>(this));
            archive(CEREAL_NVP(footBoneNames_));
            archive(CEREAL_NVP(footContactHeight_));
            archive(CEREAL_NVP(footQuakeIntensity_));
            archive(CEREAL_NVP(footQuakeDuration_secs_));
            archive(CEREAL_NVP(footQuakeInnerRadius_));
            archive(CEREAL_NVP(footQuakeOuterRadius_));
            archive(CEREAL_NVP(footstepSound_));
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
            if (version >= 5)
            {
                archive(CEREAL_NVP(footBoneNames_));
                archive(CEREAL_NVP(footContactHeight_));
                archive(CEREAL_NVP(footQuakeIntensity_));
                archive(CEREAL_NVP(footQuakeDuration_secs_));
                archive(CEREAL_NVP(footQuakeInnerRadius_));
                archive(CEREAL_NVP(footQuakeOuterRadius_));
                archive(CEREAL_NVP(footstepSound_));
            }
        }

        void BasedOnDrawgui() override;
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Npc::Enemy::Tyrannosaurus, 5);
#pragma endregion

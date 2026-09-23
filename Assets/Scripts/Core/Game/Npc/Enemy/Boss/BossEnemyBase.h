#pragma once
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../EnemyBase.h"
#include "../ShowHealthGaugeProvider/IShowHealthGaugeProvider.h"

namespace GamePlay::Ui
{
    class BossHealthGaugePresenter;
}

namespace GameCore::Npc
{
    class BossEnemyBase : public EnemyBase,
                          public Enemy::IShowHealthGaugeProvider
    {
    public:
        [[nodiscard]] const std::string& BossName() const { return bossName_; }
        [[nodiscard]] Enemy::EnemyStatus& Status() { return NetworkStatus()->Get(); }
        // ゲージUIとPresenterは EnemyFactory が生成するので、生成後にここへ差し込まれる
        void SetHealthGaugePresenter(const std::weak_ptr<GamePlay::Ui::BossHealthGaugePresenter>& presenter);

    protected:
        void ShowBossHealthGauge() override;

        template <class Archive>
        void LoadLegacyBossFields(Archive& archive, const std::uint32_t derivedVersion)
        {
            [[serialize(3)]] FIELD(Asset::PrefabGameObjectFile) bossHealthGaugePrefab_;
            if (derivedVersion >= 3) archive(CEREAL_NVP(bossHealthGaugePrefab_));
            if (derivedVersion >= 2) archive(CEREAL_NVP(bossName_));
        }

    private:
        std::string bossName_;
        std::weak_ptr<GamePlay::Ui::BossHealthGaugePresenter> bossHealthGaugePresenter_;

#pragma region Serialization Function
    public:
        void BasedOnDrawgui() override;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<EnemyBase>(this));
            archive(CEREAL_NVP(bossName_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<EnemyBase>(this));
            // v0 はゲージのプレハブを個体が持っていた。今は EnemyFactory 側にあるので読み捨てる
            [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) bossHealthGaugePrefab_;
            if (version == 0) archive(CEREAL_NVP(bossHealthGaugePrefab_));
            archive(CEREAL_NVP(bossName_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Npc::BossEnemyBase, 1);
#pragma endregion

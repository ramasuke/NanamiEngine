#pragma once
#include <memory>
#include <string>

#include "Engine/Module/Component/ComponentBase.h"

namespace GameCore::Npc
{
    class BossEnemyBase;
}

namespace GamePlay::Ui
{
    class BossHealthGauge;

    // ボスのHPとゲージUIをつなぐPresenter。EnemyFactory がゲージUIと一緒に生成する。
    // HPの購読と、ボスが消えた後のゲージ破棄はここが持ち、ボス側には残さない
    class BossHealthGaugePresenter final : public Component::ComponentBase
    {
    public:
        void Initialize(const std::weak_ptr<BossHealthGauge>& view, GameCore::Npc::BossEnemyBase& boss);
        void Show();

    private:
        void DestroyPresentation();

        std::weak_ptr<BossHealthGauge> view_;
        std::string bossName_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::BossHealthGaugePresenter, 0);

#include "BossEnemyBase.h"

#include "../../../../../GamePlay/Ui/BossHealthGauge/Ui_BossHealthGaugePresenter.h"

namespace GameCore::Npc
{
    void BossEnemyBase::SetHealthGaugePresenter(
        const std::weak_ptr<GamePlay::Ui::BossHealthGaugePresenter>& presenter)
    {
        bossHealthGaugePresenter_ = presenter;
    }

    void BossEnemyBase::ShowBossHealthGauge()
    {
        if (const auto presenter = bossHealthGaugePresenter_.lock())
            presenter->Show();
    }

    void BossEnemyBase::BasedOnDrawgui()
    {
        EnemyBase::BasedOnDrawgui();
        ImGuiHelper::OnDrawInputField("bossName_", bossName_);
    }
}

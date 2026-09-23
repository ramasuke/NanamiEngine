#include "Ui_BossHealthGaugePresenter.h"

#include "Ui_BossHealthGauge.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../Core/Game/Npc/Enemy/Boss/BossEnemyBase.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void BossHealthGaugePresenter::Initialize(
        const std::weak_ptr<BossHealthGauge>& view,
        GameCore::Npc::BossEnemyBase& boss)
    {
        view_ = view;
        bossName_ = boss.BossName();

        auto& status = boss.Status();
        const auto maxHealth = status.MaxHealth();

        // HealthObservable は購読時に現在値を流さないので、最初に一度そろえておく
        if (const auto gauge = view_.lock())
            gauge->SetHealthRate(status.Health() / maxHealth);

        status.HealthObservable().Subscribe([weakView = view_, maxHealth](const GameCore::StatusParameter::Health health)
            {
                if (const auto gauge = weakView.lock())
                    gauge->SetHealthRate(health / maxHealth);
            }).AddTo(this);

        // ボスが消えたらゲージUIごと自分も片付ける
        boss.DestroyCancellationToken().Register(
            [weakSelf = Components().Catch<BossHealthGaugePresenter>()]
            {
                if (const auto self = weakSelf.lock())
                    self->DestroyPresentation();
            });
    }

    void BossHealthGaugePresenter::Show()
    {
        const auto gauge = view_.lock();
        if (!gauge || gauge->IsEnable())
            return;

        gauge->Show(bossName_);
    }

    void BossHealthGaugePresenter::DestroyPresentation()
    {
        if (const auto gauge = view_.lock())
        {
            if (const auto gaugeObject = gauge->Entity().lock())
                gaugeObject->OnDestroy();
        }
        view_.reset();

        if (const auto self = Entity().lock())
            self->OnDestroy();
    }

    void BossHealthGaugePresenter::OnDrawGui()
    {
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BossHealthGaugePresenter);
#pragma endregion

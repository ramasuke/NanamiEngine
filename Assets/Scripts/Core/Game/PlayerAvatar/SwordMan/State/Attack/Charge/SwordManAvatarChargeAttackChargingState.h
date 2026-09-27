#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarChargeAttackChargingState final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarChargeAttackChargingState(
            const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;
        
        void EmitChargeCompleteCue() const;
        void SustainChargeShake() const;
        
        /** @brief 構えを抜けて溜め始めてからの時間 */
        [[nodiscard]] float ChargeElapsed_secs() const;

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ChargeAttackCharging; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Accept; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;

    private:
        bool isFullyCharged_ = false;
        
        std::weak_ptr<GameObject::IGameObject> chargeHoldParticle_;
        AttackTurn attackTurn_;
    };
}

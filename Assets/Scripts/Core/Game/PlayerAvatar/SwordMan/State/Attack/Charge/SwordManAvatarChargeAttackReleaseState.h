#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    /** @brief 最大まで溜めたため攻撃を解放するステート。攻撃範囲は NormalAttackArea を使う */
    class SwordManAvatarChargeAttackReleaseState final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarChargeAttackReleaseState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryChargeAttack();
        void ChangeToMoveOrIdle();

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ChargeAttackRelease; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }

    private:
        bool isAttacked_ = false;
        AttackTurn attackTurn_;
    };
}

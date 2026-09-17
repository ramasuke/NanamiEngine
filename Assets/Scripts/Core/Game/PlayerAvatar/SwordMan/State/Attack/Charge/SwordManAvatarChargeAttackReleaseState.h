#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    /** @brief 最大まで溜めたため攻撃を解放するステート。攻撃範囲は NormalAttackArea を使う */
    class SwordManAvatarChargeAttackReleaseState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarChargeAttackReleaseState)

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryChargeAttack();
        void ChangeToMoveOrIdle();

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ChargeAttackRelease; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::Momentary; }

    private:
        bool isAttacked_ = false;
    };
}

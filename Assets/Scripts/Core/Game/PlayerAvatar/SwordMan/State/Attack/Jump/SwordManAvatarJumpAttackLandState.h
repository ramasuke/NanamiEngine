#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    /** @brief ジャンプ攻撃の着地で叩きつけるステート。攻撃範囲は NormalAttackArea を使う */
    class SwordManAvatarJumpAttackLandState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarJumpAttackLandState)

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryJumpAttack();

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::JumpAttackLand; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::Momentary; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;

    private:
        bool isAttacked_ = false;
    };
}

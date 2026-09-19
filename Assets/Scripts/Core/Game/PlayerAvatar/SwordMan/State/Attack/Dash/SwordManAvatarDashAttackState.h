#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarDashAttackState final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarDashAttackState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryDashAttack();
        void ChangeToMoveOrIdle();

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::DashAttack; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }

    private:
        bool isAttacked_ = false;
        AttackTurn attackTurn_;
    };
}

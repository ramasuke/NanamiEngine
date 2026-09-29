#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarCounterAttackState final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarCounterAttackState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryCounterAttack();
        void ChangeToMoveOrIdle();

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::CounterAttack; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }

    private:
        bool isAttacked_ = false;
        AttackTurn attackTurn_;
    };
}

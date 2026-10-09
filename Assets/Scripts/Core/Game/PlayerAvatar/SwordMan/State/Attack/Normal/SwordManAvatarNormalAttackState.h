#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarNormalAttackState final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarNormalAttackState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryComboAttack();
        // NOTE: 現在の段に応じた打撃音／空振り音を鳴らす
        void PlayComboAttackSe(bool isHit) const;
        void ChangeToMoveOrIdle();
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ComboAttack; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Accept; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;

    private:
        int  currentCombo_ = 0;
        bool isAttacked_   = false;
        // NOTE: 次段の先行入力を受け付ける猶予の残り時間
        float bufferedAttackTimer_secs_ = 0.0f;
        AttackTurn attackTurn_;
    };
}

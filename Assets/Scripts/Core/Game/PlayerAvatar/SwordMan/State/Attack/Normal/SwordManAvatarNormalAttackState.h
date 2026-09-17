#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarNormalAttackState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarNormalAttackState)

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryComboAttack();
        /** @brief その段の音を鳴らす。敵に当たったかどうかで打撃音と空振り音を鳴らし分ける */
        void PlayComboAttackSe(bool isHit) const;
        void ChangeToMoveOrIdle();
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ComboAttack; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::Accept; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;

    private:
        int  currentCombo_ = 0;
        bool isAttacked_   = false;
        /** @brief NormalAttack入力の先行/後追い猶予(数フレーム分)を持たせるための残り時間 */
        float bufferedAttackTimer_secs_ = 0.0f;
        /** @brief このステートに入ってから攻撃ボタンを一度でも離したか。離していなければため攻撃の溜めへ移行できる */
        bool releasedSinceEnter_ = false;
    };
}

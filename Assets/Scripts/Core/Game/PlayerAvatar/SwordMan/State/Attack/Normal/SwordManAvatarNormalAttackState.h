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
        void ChangeToMoveOrIdle();
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ComboAttack; }

    private:
        int  currentCombo_ = 0;
        bool isAttacked_   = false;
        /** @brief NormalAttack入力の先行/後追い猶予(数フレーム分)を持たせるための残り時間 */
        float bufferedAttackTimer_secs_ = 0.0f;
    };
}

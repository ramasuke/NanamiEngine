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
        void DoUpdate() override;
        void DoExit() override;

        void TryComboAttack();
        void ChangeToMoveOrIdle();
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ComboAttack; }

        int  currentCombo_ = 0;
        bool isAttacked_   = false;
    };
}

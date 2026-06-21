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
        /** 与えたダメージを表示するTextを生成 */
        void DealDamageText(Damage::PhysicsPower power);
        void ChangeToMoveOrIdle();
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ComboAttack; }

    private:
        int  currentCombo_ = 0;
        bool isAttacked_   = false;
    };
}

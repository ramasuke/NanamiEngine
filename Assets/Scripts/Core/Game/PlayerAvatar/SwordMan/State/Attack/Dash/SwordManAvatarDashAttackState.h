#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarDashAttackState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarDashAttackState)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::DashAttack;

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        void TryDashAttack();
        void ChangeToMoveOrIdle();

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::DashAttack; }

    private:
        bool isAttacked_ = false;
    };
}

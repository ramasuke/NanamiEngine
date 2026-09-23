#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    /** @brief 空中で振りかぶってから真下へ急降下するステート。接地したら JumpAttackLand で叩きつける */
    class SwordManAvatarJumpAttackAirState final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarJumpAttackAirState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::JumpAttackAir; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;

    private:
        AttackTurn attackTurn_;
        bool       isPlunging_ = false;
    };
}

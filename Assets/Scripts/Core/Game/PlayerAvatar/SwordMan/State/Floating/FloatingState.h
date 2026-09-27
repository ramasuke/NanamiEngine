#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class FloatingState final : public SwordManAvatarStateBase
    {
    public:
        explicit FloatingState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;

        /** @brief 接地していたら、落ちてきた速さに応じた大きさの土煙を足元へ出す */
        void TryEmitLandingParticle();

        float fallSpeed_ = 0.0f; ///< 空中にいる間の最大落下速度。接地時には床に潰されて0になっているので覚えておく
        bool  hasEmittedLandingParticle_ = false; ///< 着地遷移が遅れても毎フレーム出さないよう、1回の Floating につき1回だけ出す

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Fall; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Accept; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;
    };
}

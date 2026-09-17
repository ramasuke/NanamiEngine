#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class FloatingState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(FloatingState)

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;

        /** @brief 接地していたら、落ちてきた速さに応じた大きさの土煙を足元へ出す */
        void TryEmitLandingParticle() const;

        float fallSpeed_ = 0.0f; ///< 空中にいる間の最大落下速度。接地時には床に潰されて0になっているので覚えておく

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Jump; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::Accept; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;
    };
}

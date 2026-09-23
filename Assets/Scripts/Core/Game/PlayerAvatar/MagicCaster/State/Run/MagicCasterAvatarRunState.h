#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    class RunState final : public MagicCasterAvatarStateBase
    {
    public:
        explicit RunState(const MagicCasterAvatarStateArgs& args) : MagicCasterAvatarStateBase(args) {}

    private:
        void DoEnter      () override;
        void DoUpdate     () override;
        void DoFixedUpdate() override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::Run; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Accept; }
        void VisitTransitions(IMagicCasterAvatarTransitionVisitor& visitor) const override;

        FootstepLatch footstep_;
    };
}

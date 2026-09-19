#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    class HurtState final : public MagicCasterAvatarStateBase
    {
    public:
        explicit HurtState(const MagicCasterAvatarStateArgs& args) : MagicCasterAvatarStateBase(args) {}

    private:
        void DoEnter      () override;
        void DoUpdate     () override;
        void DoFixedUpdate() override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::Hurt; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }
        void VisitTransitions(IMagicCasterAvatarTransitionVisitor& visitor) const override;
    };
}

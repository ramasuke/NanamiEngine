#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    class AvoidRollingState final : public MagicCasterAvatarStateBase
    {
    public:
        explicit AvoidRollingState(const MagicCasterAvatarStateArgs& args) : MagicCasterAvatarStateBase(args) {}

    private:
        void DoEnter      () override;
        void DoUpdate     () override;
        void DoFixedUpdate() override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::AvoidRolling; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }

        bool isAvoided_ = false;
    };
}

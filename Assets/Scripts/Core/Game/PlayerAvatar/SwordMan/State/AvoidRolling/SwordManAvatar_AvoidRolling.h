#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class AvoidRollingState final : public SwordManAvatarStateBase
    {
    public:
        explicit AvoidRollingState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::AvoidRolling; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }
        

        static constexpr float MIN_ROLL_BEFORE_COUNTER_SECS = 0.08f;

        bool isAvoided_ = false;
    };
}

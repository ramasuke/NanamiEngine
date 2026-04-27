#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class AvoidRollingState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(AvoidRollingState)

    private:
        void DoEnter () override;
        void DoUpdate() override;
        void DoExit  () override;
        
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::AvoidRolling; }
        

        bool isAvoided_ = false;
    };
}

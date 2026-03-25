#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarChattingState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarChattingState)

    private:
        void DoEnter () override;
        void DoUpdate() override;
        void DoExit  () override;
        
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Idle; }
    };
}

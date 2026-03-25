#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class DeathState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(DeathState)
        
    private:
        void DoEnter () override;
        void DoUpdate() override;
        void DoExit  () override;

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Death; }
    };
}

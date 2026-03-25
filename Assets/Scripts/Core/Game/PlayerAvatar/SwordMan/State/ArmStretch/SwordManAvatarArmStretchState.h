#pragma once
#include "../SwordManAvatarStateBase.h"
#include "../../InputAction/SwordManAvatarInputAction.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarArmStretchState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarArmStretchState)
        
    private:
        void DoEnter () override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ArmStretch; }
    };
}

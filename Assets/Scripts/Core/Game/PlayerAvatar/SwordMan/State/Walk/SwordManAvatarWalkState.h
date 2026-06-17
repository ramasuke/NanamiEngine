#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarWalkState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarWalkState)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::Walk;

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Walk; }
    };
}

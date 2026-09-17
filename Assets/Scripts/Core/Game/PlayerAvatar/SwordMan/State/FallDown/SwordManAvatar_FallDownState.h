#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class FallDownState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(FallDownState)

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::FallDown; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::None; }
    };
}

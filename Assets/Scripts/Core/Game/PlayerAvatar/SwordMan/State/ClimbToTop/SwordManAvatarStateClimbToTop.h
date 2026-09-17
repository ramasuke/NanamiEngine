#pragma once
#include "../SwordManAvatarStateBase.h"
#include "../../../../../../../../Engine/Core/Coroutine/Task/Task.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarStateClimbToTop final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarStateClimbToTop)

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ClimbToTop; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::None; }

        // Coroutine::Task<void> ClimbingAsync();
    };
}

#pragma once
#include "../SwordManAvatarStateBase.h"
#include "../../../../../../../../Engine/Core/Coroutine/Task/Task.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarStateClimbToTop final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarStateClimbToTop)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::ClimbToTop;

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ClimbToTop; }

        // Coroutine::Task<void> ClimbingAsync();
    };
}

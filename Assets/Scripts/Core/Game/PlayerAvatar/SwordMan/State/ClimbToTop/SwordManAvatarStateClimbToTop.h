#pragma once
#include "../SwordManAvatarStateBase.h"
#include "../../../../../../../../Engine/Core/Coroutine/Task/Task.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarStateClimbToTop final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarStateClimbToTop(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ClimbToTop; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::None; }

        // Coroutine::Task<void> ClimbingAsync();
    };
}

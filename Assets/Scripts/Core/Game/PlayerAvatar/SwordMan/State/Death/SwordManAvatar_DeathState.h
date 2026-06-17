#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class DeathState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(DeathState)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::Death;

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Death; }
    };
}

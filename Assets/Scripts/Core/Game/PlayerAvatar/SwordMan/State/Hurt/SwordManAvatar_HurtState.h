#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class HurtState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(HurtState)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::Hurt;

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Hurt; }
    };
}

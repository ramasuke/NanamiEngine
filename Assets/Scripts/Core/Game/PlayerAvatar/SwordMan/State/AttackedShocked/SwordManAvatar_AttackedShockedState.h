#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class AttackedShockedState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(AttackedShockedState)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::AttackedShocked;

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::AttackedShocked; }
    };
}

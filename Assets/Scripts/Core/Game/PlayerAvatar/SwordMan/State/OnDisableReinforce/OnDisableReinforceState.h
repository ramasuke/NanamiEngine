#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class OnDisableReinforceState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(OnDisableReinforceState)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::OnDisableReinforce;

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::OnDisableReinforce; }
    };
}

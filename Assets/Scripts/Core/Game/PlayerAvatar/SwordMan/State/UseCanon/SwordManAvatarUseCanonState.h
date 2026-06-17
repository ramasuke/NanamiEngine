#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarUseCannonState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarUseCannonState)
        static constexpr SwordManAvatarStateType kStateType = SwordManAvatarStateType::UseCanon;

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Idle; }
    };
}

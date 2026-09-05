#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class WakeUpState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(WakeUpState)

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Chatting; }
    };
}

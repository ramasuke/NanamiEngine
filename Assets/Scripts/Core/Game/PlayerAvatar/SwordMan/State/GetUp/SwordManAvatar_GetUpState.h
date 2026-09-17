#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class GetUpState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(GetUpState)

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::GetUp; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::None; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;
    };
}

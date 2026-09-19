#pragma once
#include "../SwordManAvatarStateBase.h"
#include "../../InputAction/SwordManAvatarInputAction.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class SwordManAvatarArmStretchState final : public SwordManAvatarStateBase
    {
    public:
        explicit SwordManAvatarArmStretchState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter () override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit  () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ArmStretch; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::None; }
    };
}

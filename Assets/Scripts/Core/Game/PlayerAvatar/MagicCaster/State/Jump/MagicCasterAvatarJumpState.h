#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    class JumpState final : public MagicCasterAvatarStateBase
    {
    public:
        DEFINE_MAGICCASTER_STATE_CONSTRUCTOR(JumpState)

    private:
        void DoEnter      () override;
        void DoUpdate     () override;
        void DoFixedUpdate() override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::Jump; }
    };
}

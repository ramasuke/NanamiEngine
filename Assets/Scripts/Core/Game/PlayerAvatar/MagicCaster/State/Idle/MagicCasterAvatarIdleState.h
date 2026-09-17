#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    class IdleState final : public MagicCasterAvatarStateBase
    {
    public:
        DEFINE_MAGICCASTER_STATE_CONSTRUCTOR(IdleState)

    private:
        void DoEnter      () override;
        void DoUpdate     () override;
        void DoFixedUpdate() override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::Idle; }
    };
}

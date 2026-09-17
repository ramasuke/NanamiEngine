#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    class WalkState final : public MagicCasterAvatarStateBase
    {
    public:
        DEFINE_MAGICCASTER_STATE_CONSTRUCTOR(WalkState)

    private:
        void DoEnter      () override;
        void DoUpdate     () override;
        void DoFixedUpdate() override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::Walk; }
    };
}

#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    // NOTE: ポータルから歩いて出てくる登場ステート。操作不可で歩行モーションだけを流す
    // NOTE: 位置は到着演出が毎フレーム書き込むので、ここでは動かさない。抜けるのも演出側がIdleへ変える
    class WarpInState final : public MagicCasterAvatarStateBase
    {
    public:
        explicit WarpInState(const MagicCasterAvatarStateArgs& args) : MagicCasterAvatarStateBase(args) {}

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::Walk; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::None; }

        FootstepLatch footstep_;
    };
}

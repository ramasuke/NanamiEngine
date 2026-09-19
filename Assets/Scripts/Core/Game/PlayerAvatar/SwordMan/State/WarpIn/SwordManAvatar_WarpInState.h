#pragma once
#include "../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    /**
     * @brief ポータルから歩いて出てくる登場ステート。操作不可で歩行モーションだけを流す
     * @note 位置は到着演出が毎フレーム書き込むので、ここでは動かさない。抜けるのも演出側がIdleへ変える
     */
    class WarpInState final : public SwordManAvatarStateBase
    {
    public:
        explicit WarpInState(const SwordManAvatarStateArgs& args) : SwordManAvatarStateBase(args) {}

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;
        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::Walk; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::None; }

    private:
        FootstepLatch footstep_;
    };
}

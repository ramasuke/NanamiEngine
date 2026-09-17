#pragma once
#include "../../SwordManAvatarStateBase.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    /**
     * @brief ため攻撃の溜め中ステート
     * @note NormalAttack中に攻撃ボタンを押し続けると遷移する。
     *       最大溜めに達してから離すと ChargeAttackRelease、溜め切る前に離すと通常コンボ(NormalAttack)の1段目になる
     */
    class SwordManAvatarChargeAttackChargingState final : public SwordManAvatarStateBase
    {
    public:
        DEFINE_STATE_CONSTRUCTOR(SwordManAvatarChargeAttackChargingState)

    private:
        void DoEnter() override;
        void DoFixedUpdate() override;
        void DoUpdate() override;
        void DoExit() override;

        /** @brief 最大溜めに達したことを知らせる SE とエフェクトを出す */
        void EmitChargeCompleteCue() const;
        /** @brief 溜め具合に応じた持続カメラ揺れを毎フレーム要求する */
        void SustainChargeShake() const;

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return AnimationType::ChargeAttackCharging; }
        [[nodiscard]] SwordManAvatarControlAcceptance ControlAcceptance() const override { return SwordManAvatarControlAcceptance::Accept; }
        void VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const override;

    private:
        /** @brief 最大溜めに達したか。達した最初のフレームで合図を1回だけ出すために使う */
        bool isFullyCharged_ = false;
        /** @brief 最大溜め保持中のオーラ。ステートを抜けるときに破棄する */
        std::weak_ptr<GameObject::IGameObject> chargeHoldParticle_;
    };
}

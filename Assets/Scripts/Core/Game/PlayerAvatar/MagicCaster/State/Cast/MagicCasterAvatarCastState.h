#pragma once
#include "../MagicCasterAvatarStateBase.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    // 魔法の中身は知らず、IMagicSpell の時間で溜めて撃つだけ
    class CastState final : public MagicCasterAvatarStateBase
    {
    public:
        explicit CastState(const MagicCasterAvatarStateArgs& args) : MagicCasterAvatarStateBase(args) {}

    private:
        void DoEnter      () override;
        void DoUpdate     () override;
        void DoFixedUpdate() override;
        void DoExit       () override;
        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return AnimationType::Cast; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }

        void SpawnCastEffect();
        /** @brief ロックオンで向きを変えている間も、詠唱の演出を足元と向きに合わせ続ける */
        void FollowCastEffect() const;

        std::shared_ptr<const Magic::IMagicSpell> spell_;
        std::weak_ptr<GameObject::IGameObject> castEffect_;
        int  slot_     = 0;
        bool hasFired_ = false;
    };
}

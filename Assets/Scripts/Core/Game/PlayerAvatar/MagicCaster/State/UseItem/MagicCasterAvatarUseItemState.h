#pragma once
#include "../MagicCasterAvatarStateBase.h"
#include "../../../Item/ItemUseAction.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
{
    // その場で止まってアイテムを使う。どのモーションかはステートごとに決まり(リモートにはステート番号だけが届くため)、時刻はアイテムが持つ
    class UseItemState final : public MagicCasterAvatarStateBase
    {
    public:
        UseItemState(const MagicCasterAvatarStateArgs& args, MagicCaster::AnimationType animation)
            : MagicCasterAvatarStateBase(args), animation_(animation) {}

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;

        [[nodiscard]] MagicCaster::AnimationType AnimationType() const override { return animation_; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }

        MagicCaster::AnimationType animation_;
        Item::ItemUseAction        action_;
    };
}

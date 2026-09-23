#pragma once
#include "../SwordManAvatarStateBase.h"
#include "../../../Item/ItemUseAction.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    // その場で止まってアイテムを使う。どのモーションかはステートごとに決まり(リモートにはステート番号だけが届くため)、時刻はアイテムが持つ
    class UseItemState final : public SwordManAvatarStateBase
    {
    public:
        UseItemState(const SwordManAvatarStateArgs& args, SwordMan::AnimationType animation)
            : SwordManAvatarStateBase(args), animation_(animation) {}

    private:
        void DoEnter      () override;
        void DoFixedUpdate() override;
        void DoUpdate     () override;
        void DoExit       () override;

        [[nodiscard]] SwordMan::AnimationType AnimationType() const override { return animation_; }
        [[nodiscard]] PlayerAvatarControlAcceptance ControlAcceptance() const override { return PlayerAvatarControlAcceptance::Momentary; }

        SwordMan::AnimationType animation_;
        Item::ItemUseAction     action_;
    };
}

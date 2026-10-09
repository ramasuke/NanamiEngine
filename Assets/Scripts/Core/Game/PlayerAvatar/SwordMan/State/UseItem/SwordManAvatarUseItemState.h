#pragma once
#include "../SwordManAvatarStateBase.h"
#include "../../../Item/ItemUseAction.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    // NOTE: その場で止まってアイテムを使うステート。使用タイミングはアイテムが持つ
    // NOTE: モーションはステートごとに固定（リモートにはステート番号しか届かない）
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

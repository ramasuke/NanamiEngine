#pragma once
#include <memory>

#include "Ui_ItemBarSource.h"

namespace GamePlay::Ui
{
    /** @brief State が宣言する操作から、アイテム欄を出すか・使えるかだけを拾う */
    template <typename TransitionVisitorT>
    class ItemBarActionCollector final : public TransitionVisitorT
    {
    public:
        using ActionType = typename TransitionVisitorT::ActionType;

        [[nodiscard]] bool IsShown () const { return isShown_;  }
        [[nodiscard]] bool IsUsable() const { return isUsable_; }

        void Action(const ActionType action, const bool isUsable) override
        {
            if (action == ActionType::CycleItem)
                isShown_ = true;
            if (action == ActionType::UseItem)
            {
                isShown_ = true;
                isUsable_ = isUsable;
            }
        }

    private:
        bool isShown_  = false;
        bool isUsable_ = false;
    };

    /// PlayerAvatarBase を継承したアバター用。TransitionVisitorT はそのアバターの State が受け取る Visitor
    template <typename AvatarT, typename TransitionVisitorT>
    class AvatarItemBarSource final : public IItemBarSource
    {
    public:
        explicit AvatarItemBarSource(const std::weak_ptr<AvatarT>& avatar)
            : avatar_(avatar)
        {
        }

        [[nodiscard]] GameCore::PlayerAvatar::ItemPouch* Pouch() const override
        {
            const auto avatar = avatar_.lock();
            return avatar ? &avatar->PlayerStatus().Pouch() : nullptr;
        }

        [[nodiscard]] GameCore::PlayerAvatar::PlayerAvatarInputDevice CurrentDevice() const override
        {
            const auto avatar = avatar_.lock();
            return avatar ? avatar->GetInputAction().CurrentDevice() : GameCore::PlayerAvatar::PlayerAvatarInputDevice::KeyboardMouse;
        }

        [[nodiscard]] ItemBarDeclaration Declaration() const override
        {
            ItemBarDeclaration declaration;
            const auto avatar = avatar_.lock();
            if (!avatar)
                return declaration;

            const auto state = avatar->GetStateMachine().CurrentStateValue();
            if (!state)
                return declaration;

            declaration.acceptance = state->ControlAcceptance();
            if (declaration.acceptance == GameCore::PlayerAvatar::PlayerAvatarControlAcceptance::Accept)
            {
                ItemBarActionCollector<TransitionVisitorT> collector;
                state->VisitTransitions(collector);
                declaration.isShown  = collector.IsShown();
                declaration.isUsable = collector.IsUsable();
            }
            return declaration;
        }

    private:
        std::weak_ptr<AvatarT> avatar_;
    };
}

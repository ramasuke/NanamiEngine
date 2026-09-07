#pragma once
#include <memory>

#include "../../../../../../../Engine/Core/Application/Window/Popup/Interface/IPopupWindow.h"
#include "../../../../../../../Engine/Core/Application/Window/Popup/Factory/PopupWindowFactory.h"
#include "../../../../../Core/Game/Npc/Friendly/Behaviour/Friendly_BehaviourTree.h"

namespace Editor::Npc::Friendly
{
    /**
     * @note シーン上で実際に動いているFriendlyNpcインスタンスのBehaviourTreeをリアルタイムに覗くPopupWindow。
     *       FriendlyNpc::OnDrawGui()の「実行中のBehaviourTreeを表示」ボタンから対象がセットされる。
     */
    class RunningFriendlyBehaviourTreeWindow final : public Core::PopupWindow::IPopupWindow
    {
    public:
        RunningFriendlyBehaviourTreeWindow();
        ::Guid& Guid() override { return guid_; }
        Core::PopupWindow::PopupWindowState OnDraw(Core::PopupWindow::PopupWindowDrawGuiContext context) override;
        void TryAddTarget(const std::weak_ptr<GameCore::Npc::Friendly::BehaviourTree>& tree);

    private:
        static int counter_;
        int id_;
        ::Guid guid_;
        std::weak_ptr<GameCore::Npc::Friendly::BehaviourTree> targetTree_;
        bool isLockedContent_ = false;
    };

    REGISTER_POPUP_WINDOW(RunningFriendlyBehaviourTreeWindow);
}

#pragma once
#include <memory>

#include "Engine/Core/Application/Window/Popup/Interface/IPopupWindow.h"
#include "Engine/Core/Application/Window/Popup/Factory/PopupWindowFactory.h"
#include "../../../../../Core/Game/Npc/Enemy/Behaviour/Enemy_BehaviourTree.h"

namespace Editor::Npc::Enemy
{
    /**
     * @note 実行中の敵の BehaviourTree をリアルタイムに覗くPopupWindow
     */
    class RunningEnemyBehaviourTreeWindow final : public Core::PopupWindow::IPopupWindow
    {
    public:
        RunningEnemyBehaviourTreeWindow();
        ::Guid& Guid() override { return guid_; }
        Core::PopupWindow::PopupWindowState OnDraw(Core::PopupWindow::PopupWindowDrawGuiContext context) override;
        void TryAddTarget(const std::weak_ptr<GameCore::Npc::Enemy::BehaviourTree>& tree);

    private:
        static int counter_;
        int id_;
        ::Guid guid_;
        std::weak_ptr<GameCore::Npc::Enemy::BehaviourTree> targetTree_;
        bool isLockedContent_ = false;
    };

    REGISTER_POPUP_WINDOW(RunningEnemyBehaviourTreeWindow, "Debug::Behaviour Tree");
}

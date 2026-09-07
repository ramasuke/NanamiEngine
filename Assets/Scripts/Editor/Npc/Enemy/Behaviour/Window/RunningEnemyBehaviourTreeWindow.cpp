#include "RunningEnemyBehaviourTreeWindow.h"

#include "ImGuiHelper.h"

int Editor::Npc::Enemy::RunningEnemyBehaviourTreeWindow::counter_ = 0;

namespace Editor::Npc::Enemy
{
    RunningEnemyBehaviourTreeWindow::RunningEnemyBehaviourTreeWindow()
    {
        id_ = counter_++;
    }

    Core::PopupWindow::PopupWindowState RunningEnemyBehaviourTreeWindow::OnDraw(Core::PopupWindow::PopupWindowDrawGuiContext context)
    {
        bool isOpen = true;
        ImGui::Begin(("Running BehaviourTree Viewer (Enemy)##" + std::to_string(id_)).c_str(), &isOpen);
        ImGui::Checkbox("isLock", &isLockedContent_);

        const auto tree = targetTree_.lock();
        if (!tree)
        {
            ImGui::TextDisabled("NPCを選択し、\"実行中のBehaviourTreeを表示\"を押してください");
        }

        ImGui::End();

        if (tree)
        {
            tree->OnDrawGraphEditorGui();
            tree->OnDrawGui();
        }

        return isOpen ? Core::PopupWindow::PopupWindowState::Open : Core::PopupWindow::PopupWindowState::Closed;
    }

    void RunningEnemyBehaviourTreeWindow::TryAddTarget(const std::weak_ptr<GameCore::Npc::Enemy::BehaviourTree>& tree)
    {
        if (!isLockedContent_)
            targetTree_ = tree;
    }
}

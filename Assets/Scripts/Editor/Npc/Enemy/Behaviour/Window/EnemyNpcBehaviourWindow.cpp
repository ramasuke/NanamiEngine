#include "EnemyNpcBehaviourWindow.h"

namespace Editor::Npc::Enemy
{
    EnemyNpcBehaviourWindow::EnemyNpcBehaviourWindow()
        : MainWindowBase(false)
    {
        
    }

    void EnemyNpcBehaviourWindow::OnSave()
    {
        for (const auto& behaviourTree : contents_ | std::views::values)
        {
            behaviourTree->OnSave();
        }
    }
    
    void EnemyNpcBehaviourWindow::OnDrawGui(Core::MainWindow::MainWindowDrawGuiContext context)
    {
        std::vector<std::shared_ptr<GameCore::Npc::Enemy::BehaviourTree>> closedContents;

        ImGui::Begin("Opened BehaviourTree Files");
        for (const auto& content : contents_ | std::views::values)
        {
            ImGui::TextUnformatted(content->GetFilePath().c_str());
            ImGui::SameLine();
            if (ImGui::Button(("Close##" + content->GetGuid().Value()).c_str()))
                closedContents.push_back(content);
        }
        ImGui::End();

        for (const auto content : contents_ | std::views::values)
        {
            content->OnDrawGraphEditorGui();
            content->OnDrawGui();
        }

        for (const auto& content : closedContents)
            RemoveContent(content);
    }
    
    void EnemyNpcBehaviourWindow::OnUpdate()
    {
        LifeCycle().OnUpdateForGame();
    }
}

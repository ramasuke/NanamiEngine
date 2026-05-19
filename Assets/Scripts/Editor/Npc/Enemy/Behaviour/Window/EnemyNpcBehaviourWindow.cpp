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
        for (const auto content : contents_ | std::views::values)
        {
            content->OnDrawGraphEditorGui();
            content->OnDrawGui();
        }
    }
    
    void EnemyNpcBehaviourWindow::OnUpdate()
    {
        LifeCycle().OnUpdateForGame();
    }
}

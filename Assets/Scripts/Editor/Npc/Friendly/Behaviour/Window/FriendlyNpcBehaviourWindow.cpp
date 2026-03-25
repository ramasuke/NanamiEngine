#include "FriendlyNpcBehaviourWindow.h"

namespace Editor::Npc::Friendly
{
    void FriendlyNpcBehaviourWindow::OnSave()
    {
        for (const auto& behaviourTree : contents_ | std::views::values)
        {
            behaviourTree->OnSave();
        }
    }
    
    void FriendlyNpcBehaviourWindow::OnDrawGui(Core::MainWindow::MainWindowDrawGuiContext context)
    {
        for (const auto content : contents_ | std::views::values)
        {
            content->OnDrawGraphEditorGui();
            content->OnDrawGui();
        }
    }
    
    void FriendlyNpcBehaviourWindow::OnUpdate()
    {
        LifeCycle().OnUpdateForGame();
    }
}

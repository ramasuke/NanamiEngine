#include "AnimatorWindow.h"
#include <ranges>

namespace NanamiEngine::Core::MainWindow
{
    AnimatorWindow::AnimatorWindow()
        : MainWindowBase(false)
    {
    }

    void AnimatorWindow::OnSave()
    {
        for (const auto& animationTree : contents_ | std::views::values)
        {
            animationTree->OnSave();
        }
    }
    
    void AnimatorWindow::OnDrawGui(MainWindowDrawGuiContext context)
    {
        for (const auto content : contents_ | std::views::values)
        {
            content->OnDrawGraphEditorGui();
            content->OnDrawGui();
        }
    }
    
    void AnimatorWindow::OnUpdate()
    {
        LifeCycle().OnUpdateForGame();
    }
}

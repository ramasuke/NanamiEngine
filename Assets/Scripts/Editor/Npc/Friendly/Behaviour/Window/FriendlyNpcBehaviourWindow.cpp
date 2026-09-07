#include "FriendlyNpcBehaviourWindow.h"

namespace Editor::Npc::Friendly
{
    FriendlyNpcBehaviourWindow::FriendlyNpcBehaviourWindow()
        : MainWindowBase(false)
    {
    }

    void FriendlyNpcBehaviourWindow::OnSave()
    {
        for (const auto& behaviourTree : contents_ | std::views::values)
        {
            behaviourTree->OnSave();
        }
    }
    
    void FriendlyNpcBehaviourWindow::OnDrawGui(Core::MainWindow::MainWindowDrawGuiContext context)
    {
        std::vector<std::shared_ptr<GameCore::Npc::Friendly::BehaviourTree>> closedContents;

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
    
    void FriendlyNpcBehaviourWindow::OnUpdate()
    {
        LifeCycle().OnUpdateForGame();
    }
}

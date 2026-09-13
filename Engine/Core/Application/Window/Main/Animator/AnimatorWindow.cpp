#include "AnimatorWindow.h"
#include <memory>
#include <ranges>
#include <vector>

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
        std::vector<std::shared_ptr<AnimationTree::AnimationTree>> closedContents;

        ImGui::Begin("Opened AnimationTree Files");
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

        // 描画ループ中に contents_ を書き換えないよう、閉じる操作はループ後に行う（未保存の編集は破棄）
        for (const auto& content : closedContents)
            RemoveContent(content);
    }
    
    void AnimatorWindow::OnUpdate()
    {
        LifeCycle().OnUpdateForGame();
    }
}

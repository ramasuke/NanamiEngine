#include "PrefabViewWindow.h"

#include "../../../../../Module/Scene/GameObject/SceneGameObject/SceneGameObject.h"

Core::MainWindow::PrefabViewWindow::PrefabViewWindow()
    : MainWindowBase(false)
{
    
}

void Core::MainWindow::PrefabViewWindow::OnUpdate()
{
    LifeCycle().OnUpdateForEditor();
    camera_.OnUpdate();
}

void Core::MainWindow::PrefabViewWindow::OnDrawGui(MainWindowDrawGuiContext context)
{
    ImGui::Begin("Prefab");
    OnDrawAddChildGameObjectButton();

    // i 番目を識別するためのカウンタ
    size_t index = 0;
    std::optional<size_t> removeIndex = std::nullopt;

    for (const auto& gameObject : contents_ | std::views::values)
    {
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::Button("Delete"))
        {
            removeIndex = index;
        }
        ImGui::SameLine();
        gameObject->OnDrawTreeGui();
        ImGui::PopID();
        ++index;
    }

    if (removeIndex.has_value())
    {
        auto it = contents_.begin();
        std::advance(it, *removeIndex);
        contents_.erase(it);
    }
    
    ImGui::End();
}

void Core::MainWindow::PrefabViewWindow::OnDrawAddChildGameObjectButton() const
{
    if (ImGui::Button("AddGameObject"))
    {
        const auto childGameObject = std::make_shared<Scene::SceneGameObject>();
        childGameObject->InitGameObject(std::weak_ptr<GameObject::IGameObject>(), childGameObject);
        
        const auto it = contents_.begin();
        const auto firstContent = it->second;
        firstContent->Transform().AddChild(childGameObject);
    }
}

void Core::MainWindow::PrefabViewWindow::OnSave()
{
    for (const auto& gameObject : contents_ | std::views::values)
    {
        gameObject->OnSave();
    }   
}

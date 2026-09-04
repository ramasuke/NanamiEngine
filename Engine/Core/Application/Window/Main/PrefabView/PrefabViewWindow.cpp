#include "PrefabViewWindow.h"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>

#include "../../../../../Module/Scene/GameObject/SceneGameObject/SceneGameObject.h"

namespace
{
    /** @brief haystackにneedleが含まれるか大文字小文字を無視して判定する */
    bool ContainsCaseInsensitive(const std::string_view haystack, const std::string_view needle)
    {
        if (needle.empty())
            return true;

        const auto equalsIgnoreCase = [](const char lhs, const char rhs)
        {
            return std::tolower(static_cast<unsigned char>(lhs)) ==
                   std::tolower(static_cast<unsigned char>(rhs));
        };

        return !std::ranges::search(haystack, needle, equalsIgnoreCase).empty();
    }
}

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

    {
        const bool hasSearchText = prefabSearchBuffer_[0] != '\0';
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (hasSearchText ? 55.0f : 0.0f));
        ImGui::InputTextWithHint("##PrefabSearch", "Search...", prefabSearchBuffer_, sizeof(prefabSearchBuffer_));
        if (hasSearchText)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear##PrefabSearch"))
            {
                prefabSearchBuffer_[0] = '\0';
            }
        }
    }
    const std::string prefabSearchText = prefabSearchBuffer_;

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

        if (prefabSearchText.empty())
        {
            gameObject->OnDrawTreeGui();
        }
        else
        {
            // 検索中は階層を無視して、子孫まで含めた全GameObjectから名前がマッチするものをフラットに一覧表示する
            const auto drawIfMatches = [&prefabSearchText](const std::shared_ptr<GameObject::IGameObject>& target)
            {
                if (target && ContainsCaseInsensitive(target->Name(), prefabSearchText))
                    target->OnDrawTreeGui(false);
            };

            drawIfMatches(gameObject);
            for (const auto& child : gameObject->Transform().GetAllChildren())
            {
                drawIfMatches(child);
            }
        }

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

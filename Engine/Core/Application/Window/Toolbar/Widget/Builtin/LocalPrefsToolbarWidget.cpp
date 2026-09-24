#include "LocalPrefsToolbarWidget.h"

#include <map>
#include <string>
#include <vector>

#include "ImGuiHelper.h"
#include "../EditorToolbarWidgetRegistry.h"
#include "../../../../../../Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"

namespace NanamiEngine::Core::Toolbar
{
    void LocalPrefsToolbarWidget::OnDraw(EditorToolbarWidgetContext&)
    {
        if (ImGui::Button("LocalPrefs"))
        {
            ImGui::OpenPopup("LocalPrefsWindow");
        }

        if (ImGui::BeginPopup("LocalPrefsWindow"))
        {
            const auto& prefsList = Module::LocalPrefs::Editor::LocalPrefsRegistry::GetInstance().GetPrefsList();

            // subPath をカテゴリキーとしてグループ化 (アルファベット順、空文字は "General")
            std::map<std::string, std::vector<size_t>> categoryMap;
            for (size_t i = 0; i < prefsList.size(); ++i)
                categoryMap[prefsList[i].subPath].push_back(i);

            for (const auto& [subPath, indices] : categoryMap)
            {
                const std::string header = subPath.empty() ? "General" : subPath;
                if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (size_t idx : indices)
                        prefsList[idx].drawEditGui();
                }
            }

            ImGui::EndPopup();
        }
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(LocalPrefsToolbarWidget, 600)
}

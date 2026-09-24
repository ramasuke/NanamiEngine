#include "BuildSettingsToolbarWidget.h"

#include "ImGuiHelper.h"
#include "../EditorToolbarWidgetRegistry.h"
#include "../../../../Build/GameBuilder.h"
#include "../../../Popup/BuildSettings/BuildSettingsWindow.h"
#include "../../../Popup/Group/PopupWindowGroup.h"

namespace NanamiEngine::Core::Toolbar
{
    void BuildSettingsToolbarWidget::OnDraw(EditorToolbarWidgetContext& context)
    {
        if (ImGui::Button("Build Settings"))
        {
            context.popupWindows.Catch<PopupWindow::BuildSettingsWindow>().front()->RequestFocus();
        }

        // Build Settings を閉じていても進み具合が分かるよう、ビルド中はツールバーにも出す
        if (auto& gameBuilder = Application::Build::GameBuilder::Instance(); gameBuilder.IsBusy())
        {
            ImGui::SameLine();
            ImGui::Text("Building: %s %s", gameBuilder.PhaseLabel(), gameBuilder.ElapsedLabel().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Cancel Build"))
            {
                gameBuilder.Cancel();
            }
        }
    }

    REGISTER_EDITOR_TOOLBAR_WIDGET(BuildSettingsToolbarWidget, 300)
}
